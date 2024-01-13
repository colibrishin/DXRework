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
        if (bool check = m_cached_components_.find(accessor, COM_T_STATE))
        {
            const auto& states = accessor->second;

            for (const auto& state : states | std::views::values)
            {
                const auto locked = state.lock();
                locked->SetActive(false);
            }
        }
    }

    void Scene::Initialize()
    {
        Renderable::Initialize();

        for (int i = 0; i < LAYER_MAX; ++i)
        {
            m_layers.emplace_back(boost::make_shared<Layer>(static_cast<eLayerType>(i)));
        }

        const auto camera = CreateGameObject<Objects::Camera>(LAYER_CAMERA).lock();
        m_mainCamera_           = camera;
        m_main_camera_local_id_ = camera->GetLocalID();

        const auto light1 = CreateGameObject<Objects::Light>(LAYER_LIGHT).lock();
        light1->GetComponent<Components::Transform>().lock()->SetLocalPosition(Vector3(5.f, 2.f, 5.f));

        const auto light2 = CreateGameObject<Objects::Light>(LAYER_LIGHT).lock();
        light2->GetComponent<Components::Transform>().lock()->SetLocalPosition(Vector3(-5.f, 2.f, 5.f));

        Initialize_INTERNAL();

        GetTaskScheduler().AddTask(
            TASK_INIT_SCENE,
            {}, [this](const std::vector<std::any>&, const float)
        {
            InitializeFinalize();
        });
    }

    void Scene::InitializeFinalize()
    {
#ifdef _DEBUG
        DisableControllers();
        const auto observer = CreateGameObject<Objects::Observer>(LAYER_UI).lock();
        m_observer_         = observer;
        // todo: maybe adding child to observer rather than binding to object?
        observer->AddChild(GetMainCamera());
#endif // _DEBUG
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

    void Scene::AddObjectFinalize(const eLayerType layer, const StrongObject& obj)
    {
        // add object to scene
        m_layers[layer]->AddGameObject(obj);
        m_cached_objects_.emplace(obj->GetID(), obj);

        if (layer == LAYER_LIGHT && obj->GetObjectType() != DEF_OBJ_T_LIGHT)
        {
            static_assert("Only light object can be added to light layer");
        }
        else if (layer == LAYER_CAMERA && obj->GetObjectType() != DEF_OBJ_T_CAMERA)
        {
            static_assert("Only camera object can be added to camera layer");
        }

        if (obj->GetObjectType() == DEF_OBJ_T_LIGHT)
        {
            RegisterLightToManager(obj->GetSharedPtr<Objects::Light>());
        }

        if (const auto tr = obj->GetComponent<Components::Transform>().lock())
        {
            m_object_position_tree_.Insert(obj);
        }

        for (const auto& comp : obj->GetAllComponents())
        {
            AddCacheComponent(comp.lock());
        }
    }

    void Scene::RemoveObjectFinalize(const GlobalEntityID id, eLayerType layer)
    {
        WeakObject obj;

        {
            ConcurrentWeakObjGlobalMap::const_accessor acc;

            if (!m_cached_objects_.find(acc, id))
            {
                // This is not intended to happen.
                throw std::runtime_error("object removal is called twice.");
            }

            obj = acc->second;
        }

        if (layer == LAYER_LIGHT)
        {
            UnregisterLightFromManager(obj.lock()->GetSharedPtr<Objects::Light>());
        }

        if (const auto locked = obj.lock())
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

        for (const auto& comp : obj.lock()->GetAllComponents())
        {
            ConcurrentWeakComRootMap::accessor comp_acc;

            if (m_cached_components_.find(comp_acc, comp.lock()->GetComponentType()))
            {
                comp_acc->second.erase(comp.lock()->GetID());
            }
        }

        m_cached_objects_.erase(id);
        m_assigned_actor_ids_.erase(obj.lock()->GetLocalID());
        m_layers[layer]->RemoveGameObject(id);
    }

    void Scene::RemoveGameObject(const GlobalEntityID id, eLayerType layer)
    {
        ConcurrentWeakObjGlobalMap::const_accessor acc;

        if (!m_cached_objects_.find(acc, id))
        {
            return;
        }

        if (!m_layers[layer]->GetGameObject(id).lock())
        {
            return;
        }

        // This object is already flagged to be deleted.
        if (acc->second.lock()->IsGarbage())
        {
            return;
        }

        acc->second.lock()->SetGarbage(true);

        GetTaskScheduler().AddTask(
                                   TASK_REM_OBJ,
                                   {},
                                   [this, id, layer](const std::vector<std::any>&, const float)
                                   {
                                       RemoveObjectFinalize(id, layer);
                                   });
    }

    ConcurrentWeakObjVec Scene::GetGameObjects(eLayerType layer) const
    {
        return m_layers[layer]->GetGameObjects();
    }

    WeakCamera Scene::GetMainCamera() const
    {
        return m_mainCamera_;
    }

    const Octree& Scene::GetObjectTree()
    {
        return m_object_position_tree_;
    }

    WeakObject Scene::FindGameObject(GlobalEntityID id) const
    {
        ConcurrentWeakObjGlobalMap::const_accessor acc;

        if (m_cached_objects_.find(acc, id))
        {
            return acc->second;
        }

        return {};
    }

    WeakObject Scene::FindGameObjectByLocalID(LocalActorID id) const
    {
        ConcurrentLocalGlobalIDMap::const_accessor actor_acc;

        if (m_assigned_actor_ids_.find(actor_acc, id))
        {
            ConcurrentWeakObjGlobalMap::const_accessor acc;

            if (m_cached_objects_.find(acc, actor_acc->second))
            {
                return acc->second;
            }
        }

        return {};
    }

    void Scene::AddCacheComponent(const StrongComponent& component)
    {
        ConcurrentWeakObjGlobalMap::const_accessor acc;

        if (m_cached_objects_.find(acc, component->GetOwner().lock()->GetID()))
        {
            ConcurrentWeakComRootMap::accessor comp_acc;

            if (m_cached_components_.find(comp_acc, component->GetComponentType()))
            {
                comp_acc->second.emplace(component->GetID(), component);
            }
            else
            {
                m_cached_components_.insert(comp_acc, component->GetComponentType());
                comp_acc->second.emplace(component->GetID(), component);
            }
        }
    }

    eSceneType Scene::GetType() const
    {
        return m_type_;
    }

    Scene::Scene(const eSceneType type)
    : m_main_camera_local_id_(g_invalid_id),
      m_type_(type),
      m_object_position_tree_() {}

    void Scene::Synchronize(const StrongScene& scene)
    {
        const auto sync = [this](const std::vector<std::any>& params, const float dt)
        {
            const auto scene = std::any_cast<StrongScene>(params[0]);

            m_layers = scene->m_layers;

            m_main_camera_local_id_ = scene->m_main_camera_local_id_;
            m_mainCamera_           = scene->m_mainCamera_;
            m_cached_objects_       = scene->m_cached_objects_;
            m_cached_components_    = scene->m_cached_components_;
            m_assigned_actor_ids_   = scene->m_assigned_actor_ids_;
        };

        GetTaskScheduler().AddTask(TASK_SYNC_SCENE, {scene}, sync);
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

        m_object_position_tree_.Update();
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

        // todo: scene load is assuming that this scene will be the active scene.
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
        const auto observer = CreateGameObject<Objects::Observer>(LAYER_UI).lock();
        m_observer_         = observer;
        observer->AddChild(GetMainCamera());
#endif
    }
} // namespace Engine
