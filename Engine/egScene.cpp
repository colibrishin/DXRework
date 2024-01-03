#include "pch.h"
#include "egScene.hpp"
#include "egCamera.h"
#include "egIStateController.h"
#include "egLight.h"
#include "egManagerHelper.hpp"
#include "egObserver.h"

SERIALIZER_ACCESS_IMPL(
                       Engine::Scene,
                       _ARTAG(_BSTSUPER(Renderable))
                       _ARTAG(m_main_camera_local_id_) 
                       _ARTAG(m_layers)
                       _ARTAG(m_type_))

namespace Engine
{
    void Scene::DisableControllers()
    {
        // Note: accessor should be destroyed in used context, if not, it will cause deadlock.
        ConcurrentWeakComRootMap::accessor accessor;
        bool                               check  = m_cached_components_.find(accessor, COM_T_STATE);
        const auto&                        states = accessor->second;

        for (const auto& state : states | std::views::values)
        {
            const auto locked = state.lock();
            locked->SetActive(false);
        }
    }

    void Scene::Initialize()
    {
        for (int i = 0; i < LAYER_MAX; ++i)
        {
            m_layers.emplace(static_cast<eLayerType>(i), boost::make_shared<Layer>(static_cast<eLayerType>(i)));
        }

        const auto camera = CreateGameObject<Objects::Camera>(LAYER_CAMERA).lock();
        m_mainCamera_           = camera;
        m_main_camera_local_id_ = camera->GetLocalID();

        const auto light1 = CreateGameObject<Objects::Light>(LAYER_LIGHT).lock();
        light1->GetComponent<Components::Transform>().lock()->SetLocalPosition(Vector3(5.f, 2.f, 5.f));

        const auto light2 = CreateGameObject<Objects::Light>(LAYER_LIGHT).lock();
        light2->GetComponent<Components::Transform>().lock()->SetLocalPosition(Vector3(-5.f, 2.f, 5.f));

        Initialize_INTERNAL();

#ifdef _DEBUG
        DisableControllers();
        const auto observer = CreateGameObject<Objects::Observer>(LAYER_UI);
        m_observer_         = observer;
        // todo: maybe adding child to observer rather than binding to object?
        GetMainCamera().lock()->BindObject(m_observer_);
#endif
    }

    void Scene::AssignLocalIDToObject(const StrongObject& obj)
    {
        LocalActorID id = 0;

        while (true)
        {
            if (id == g_invalid_id)
            {
                throw std::exception("Actor ID overflow");
            }

            ConcurrentLocalGlobalIDMap::const_accessor acc;
            if (!m_assigned_actor_ids_.find(acc, id))
            {
                m_assigned_actor_ids_.emplace(id, obj->GetID());
                break;
            }

            ++id;
        }

        obj->GetSharedPtr<Abstract::Actor>()->SetLocalID(id);
    }

    void Scene::RegisterLightToManager(const StrongLight& obj)
    {
        GetShadowManager().RegisterLight(obj->GetSharedPtr<Objects::Light>());
    }

    void Scene::UnregisterLightFromManager(const StrongLight& obj)
    {
        GetShadowManager().UnregisterLight(obj);
    }

    void Scene::RemoveObjectFromCache(const WeakObject& obj)
    {
        m_cached_objects_.erase(obj.lock()->GetID());
        m_assigned_actor_ids_.erase(obj.lock()->GetLocalID());
    }

    void Scene::RemoveObjectFromOctree(const WeakObject& obj)
    {
        if (const auto locked = obj.lock())
        {
            const auto tr      = locked->GetComponent<Components::Transform>().lock();
            bool       updated = false;

            if (tr)
            {
                const auto prev_pos =
                        tr->GetWorldPreviousPosition() + Vector3::One * g_octree_negative_round_up;
                const auto pos =
                        tr->GetWorldPreviousPosition() + Vector3::One * g_octree_negative_round_up;

                auto& prev_pos_set = m_object_position_tree_(
                                                             static_cast<int>(prev_pos.x), static_cast<int>(prev_pos.y),
                                                             static_cast<int>(prev_pos.z));
                auto& pos_set = m_object_position_tree_(
                                                        static_cast<int>(pos.x),
                                                        static_cast<int>(pos.y),
                                                        static_cast<int>(pos.z));

                if (prev_pos_set.contains(obj))
                {
                    prev_pos_set.erase(obj);
                    updated = true;
                }
                if (pos_set.contains(obj))
                {
                    pos_set.erase(obj);
                    updated = true;
                }
            }

            if (!updated)
            {
                // @todo: add task for refreshing octree.
            }
        }
    }

    void Scene::RemoveGameObject(const GlobalEntityID id, eLayerType layer)
    {
        ConcurrentWeakObjGlobalMap::const_accessor acc;

        if (!m_cached_objects_.find(acc, id))
        {
            return;
        }

        RemoveObjectFromOctree(acc->second);
        RemoveObjectFromCache(acc->second);

        if (layer == LAYER_LIGHT)
        {
            UnregisterLightFromManager(acc->second.lock()->GetSharedPtr<Objects::Light>());
        }

        if (const auto locked = acc->second.lock())
        {
            if (const auto parent = locked->GetParent().lock())
            {
                parent->DetachChild(locked->GetLocalID());
            }

            if (locked->GetChildren().size() > 0)
            {
                for (const auto& child : locked->GetChildren())
                {
                    RemoveGameObject(child.lock()->GetID(), child.lock()->GetLayer());
                }
            }
        }

        m_cached_objects_.erase(id);
        m_layers[layer]->RemoveGameObject(id);
        m_assigned_actor_ids_.erase(acc->second.lock()->GetLocalID());
    }

    ConcurrentWeakObjVec Scene::GetGameObjects(eLayerType layer)
    {
        return m_layers[layer]->GetGameObjects();
    }

    eSceneType Scene::GetType() const
    {
        return m_type_;
    }

    void     Scene::UpdatePosition(const WeakObject& obj)
    {
        if (const auto obj_ptr = obj.lock())
        {
            const auto tr = obj_ptr->GetComponent<Components::Transform>().lock();

            if (!tr)
            {
                GetDebugger().Log(L"Object has no transform component");
                return;
            }

            const auto prev_pos =
                    VectorElementAdd(tr->GetWorldPreviousPosition(), g_octree_negative_round_up);
            const auto pos =
                    VectorElementAdd(tr->GetWorldPreviousPosition(), g_octree_negative_round_up);

            if (!VectorElementInRange(prev_pos, g_max_map_size) ||
                !VectorElementInRange(pos, g_max_map_size))
            {
                GetDebugger().Log(L"Object position is out of range");
                return;
            }

            const auto delta = prev_pos - pos;

            if (std::fabsf(delta.x) <= g_epsilon && std::fabsf(delta.y) <= g_epsilon &&
                std::fabsf(delta.z) <= g_epsilon)
            {
                return;
            }

            auto& prev_pos_set = m_object_position_tree_(
                                                         static_cast<int>(prev_pos.x),
                                                         static_cast<int>(prev_pos.y),
                                                         static_cast<int>(prev_pos.z));
            auto& pos_set = m_object_position_tree_(
                                                    static_cast<int>(pos.x),
                                                    static_cast<int>(pos.y),
                                                    static_cast<int>(pos.z));

            if (prev_pos_set.contains(obj))
            {
                prev_pos_set.erase(obj);
            }

            if (!pos_set.contains(obj))
            {
                pos_set.insert(obj);
            }
        }
    }

    void Scene::GetNearestObjects(
        const Vector3&           pos,
        std::vector<WeakObject>& out)
    {
        const auto pos_rounded = VectorElementAdd(pos, g_octree_negative_round_up);

        if (!VectorElementInRange(pos_rounded, g_max_map_size))
        {
            GetDebugger().Log(L"Position is out of range");
            return;
        }

        const auto& pos_set = m_object_position_tree_(
                                                      static_cast<int>(pos_rounded.x), static_cast<int>(pos_rounded.y),
                                                      static_cast<int>(pos_rounded.z));

        for (const auto& obj : pos_set)
        {
            if (const auto obj_ptr = obj.lock())
            {
                out.push_back(obj_ptr);
            }
        }
    }

    void Scene::GetNearbyObjects(
        const Vector3&           pos, const UINT range,
        std::vector<WeakObject>& out)
    {
        const auto pos_rounded = VectorElementAdd(pos, g_octree_negative_round_up);

        for (auto i = static_cast<UINT>(pos_rounded.x) - range;
             i < static_cast<UINT>(pos_rounded.x) + range; ++i)
        {
            for (auto j = static_cast<UINT>(pos_rounded.y) - range;
                 j < static_cast<UINT>(pos_rounded.y) + range; ++j)
            {
                for (auto k = static_cast<UINT>(pos_rounded.z) - range;
                     k < static_cast<UINT>(pos_rounded.z) + range; ++k)
                {
                    if (!VectorElementInRange(pos_rounded, g_max_map_size))
                    {
                        GetDebugger().Log(L"Position is out of range");
                        continue;
                    }

                    const auto& set = m_object_position_tree_(i, j, k);

                    for (const auto& obj : set)
                    {
                        if (const auto obj_ptr = obj.lock())
                        {
                            out.push_back(obj_ptr);
                        }
                    }
                }
            }
        }
    }

    void Scene::SearchObjects(
        const Vector3&                                        pos, const Vector3& dir,
        std::set<WeakObject, WeakComparer<Abstract::Object>>& out, int            exhaust)
    {
        auto  pos_rounded        = VectorElementAdd(pos, g_octree_negative_round_up);
        float accumulated_length = 0.f;

        while (static_cast<int>(accumulated_length) < exhaust)
        {
            const auto& current_tree = m_object_position_tree_(
                                                               static_cast<int>(pos_rounded.x),
                                                               static_cast<int>(pos_rounded.y),
                                                               static_cast<int>(pos_rounded.z));

            for (const auto& obj : current_tree)
            {
                if (const auto obj_ptr = obj.lock())
                {
                    out.insert(obj_ptr);
                }
            }

            pos_rounded += dir;
            accumulated_length += dir.Length();
        }
    }

    Scene::Scene()
    : m_main_camera_local_id_(g_invalid_id),
      m_type_(),
      m_object_position_tree_(g_max_map_size, {}) {}

    void Scene::Synchronize(const StrongScene& scene)
    {
        GetTaskScheduler().AddTask(
                                   [this, scene](const float& dt)
                                   {
                                       GetDebugger().Log(L"Scene synchronization started.");

                                       m_layers = scene->m_layers;

                                       m_main_camera_local_id_ = scene->m_main_camera_local_id_;
                                       m_mainCamera_           = scene->m_mainCamera_;
                                       m_cached_objects_       = scene->m_cached_objects_;
                                       m_cached_components_    = scene->m_cached_components_;
                                       m_object_position_tree_ = scene->m_object_position_tree_;
                                       m_assigned_actor_ids_   = scene->m_assigned_actor_ids_;
                                   });
    }

    void Scene::OpenLoadPopup(bool& is_load_open)
    {
        if (is_load_open)
        {
            if (ImGui::Begin(
                             "Load Scene", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize))
            {
                static char buf[256] = {0};

                ImGui::InputText("Filename", buf, IM_ARRAYSIZE(buf));

                if (ImGui::Button("Load"))
                {
                    const auto scene = Serializer::Deserialize<Scene>(buf);

                    Synchronize(scene);

                    is_load_open = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();

                if (ImGui::Button("Cancel"))
                {
                    is_load_open = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::End();
            }
        }
    }

    void Scene::AddCustomObject() {}

    void Scene::PreUpdate(const float& dt)
    {
        for (int i = LAYER_NONE; i < LAYER_MAX; ++i)
        {
            m_layers[static_cast<eLayerType>(i)]->PreUpdate(dt);
        }
    }

    void Scene::Update(const float& dt)
    {
        for (int i = LAYER_NONE; i < LAYER_MAX; ++i)
        {
            m_layers[static_cast<eLayerType>(i)]->Update(dt);
        }
    }

    void Scene::PreRender(const float& dt)
    {
        for (int i = LAYER_NONE; i < LAYER_MAX; ++i)
        {
            m_layers[static_cast<eLayerType>(i)]->PreRender(dt);
        }
    }

    void Scene::Save()
    {
        auto name = std::to_string(GetID()) + " " + typeid(*this).name();
        std::ranges::replace(name, ' ', '_');
        std::ranges::replace(name, ':', '_');
        name += ".txt";

        Serializer::Serialize(name, GetSharedPtr<Scene>());
    }

    void Scene::Render(const float& dt)
    {
        for (int i = LAYER_NONE; i < LAYER_MAX; ++i)
        {
            m_layers[static_cast<eLayerType>(i)]->Render(dt);
        }

#ifdef _DEBUG
        static bool is_load_open = false;

        if (ImGui::Begin(GetTypeName().c_str()), nullptr, ImGuiWindowFlags_MenuBar)
        {
            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("Add"))
                {
                    if (ImGui::MenuItem("Camera"))
                    {
                        CreateGameObject<Objects::Camera>(LAYER_CAMERA);
                    }

                    if (ImGui::MenuItem("Light"))
                    {
                        CreateGameObject<Objects::Light>(LAYER_LIGHT);
                    }

                    AddCustomObject();

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Load"))
                {
                    if (ImGui::MenuItem("Scene"))
                    {
                        is_load_open = true;
                    }

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Save"))
                {
                    if (ImGui::MenuItem("Scene"))
                    {
                        Save();
                    }

                    ImGui::EndMenu();
                }

                ImGui::EndMainMenuBar();
            }

            OpenLoadPopup(is_load_open);

            for (int i = LAYER_NONE; i < LAYER_MAX; ++i)
            {
                for (const auto& obj : GetGameObjects(static_cast<eLayerType>(i)))
                {
                    if (const auto obj_ptr = obj.lock())
                    {
                        const auto unique_name = obj_ptr->GetName() + " " +
                                                 obj_ptr->GetTypeName() + " " +
                                                 std::to_string(obj_ptr->GetID());

                        if (ImGui::Button(unique_name.c_str()))
                        {
                            obj_ptr->SetImGuiOpen(!obj_ptr->GetImGuiOpen());
                        }

                        if (obj_ptr->GetImGuiOpen())
                        {
                            obj_ptr->OnImGui();
                        }
                    }
                }
            }

            ImGui::End();
        }
#endif
    }

    void Scene::FixedUpdate(const float& dt)
    {
        for (int i = LAYER_NONE; i < LAYER_MAX; ++i)
        {
            m_layers[static_cast<eLayerType>(i)]->FixedUpdate(dt);
        }
    }

    void Scene::PostRender(const float& dt)
    {
        for (int i = LAYER_NONE; i < LAYER_MAX; ++i)
        {
            m_layers[static_cast<eLayerType>(i)]->PostRender(dt);
        }
    }

    void Scene::PostUpdate(const float& dt)
    {
        for (int i = LAYER_NONE; i < LAYER_MAX; ++i)
        {
            m_layers[static_cast<eLayerType>(i)]->PostUpdate(dt);
        }
    }

    void Scene::OnDeserialized()
    {
        Renderable::OnDeserialized();

        auto ui = m_layers[LAYER_UI]->GetGameObjects();

        // remove observer of previous scene
        for (int i = 0; i < ui.size(); ++i)
        {
            if (const auto locked = ui[i].lock())
            {
                if (locked->GetObjectType() == DEF_OBJ_T_OBSERVER)
                {
                    m_layers[LAYER_UI]->RemoveGameObject(ui[i].lock()->GetID());
                    i--;
                }
            }
        }

        GetShadowManager().Reset();

        auto lights = m_layers[LAYER_LIGHT]->GetGameObjects();

        for (const auto& light : lights)
        {
            RegisterLightToManager(light.lock()->GetSharedPtr<Objects::Light>());
        }

        // rebuild cache
        for (int i = 0; i < LAYER_MAX; ++i)
        {
            m_layers[static_cast<eLayerType>(i)]->OnDeserialized();

            for (const auto& obj :
                 m_layers[static_cast<eLayerType>(i)]->GetGameObjects())
            {
                m_cached_objects_.emplace(obj.lock()->GetID(), obj);
                obj.lock()->SetScene(GetSharedPtr<Scene>());
                obj.lock()->SetLayer(static_cast<eLayerType>(i));
                m_assigned_actor_ids_.emplace(obj.lock()->GetLocalID(), obj.lock()->GetID());

                for (const auto& comp : obj.lock()->GetAllComponents())
                {
                    if (ConcurrentWeakComRootMap::accessor acc; 
                        m_cached_components_.find(acc, comp.lock()->GetComponentType()))
                    {
                        acc->second.emplace(comp.lock()->GetID(), comp);
                    }
                    else
                    {
                        m_cached_components_.insert(acc, comp.lock()->GetComponentType());
                        acc->second.emplace(comp.lock()->GetID(), comp);
                    }
                }
            }
        }

        const auto& cameras = m_layers[LAYER_CAMERA]->GetGameObjects();

        const auto                   it = std::ranges::find_if(
         cameras, [this](const auto& obj)
         {
             if (obj.lock()->GetLocalID() == m_main_camera_local_id_)
             {
                 return true;
             }

             return false;
         });

        if (it != cameras.end()) m_mainCamera_ = it->lock()->GetSharedPtr<Objects::Camera>();
        else m_mainCamera_                     = cameras.begin()->lock()->GetSharedPtr<Objects::Camera>();

        // @todo: rebuild octree.

#ifdef _DEBUG
        // remove controller if it is debug state
        DisableControllers();

        const auto observer = CreateGameObject<Objects::Observer>(LAYER_UI);
        m_observer_         = observer;
        GetMainCamera().lock()->BindObject(m_observer_);
#endif
    }
} // namespace Engine
