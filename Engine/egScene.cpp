#include "pch.hpp"
#include "egScene.hpp"
#include "egCamera.hpp"
#include "egIStateController.hpp"
#include "egLight.hpp"
#include "egManagerHelper.hpp"
#include "egObserver.hpp"

SERIALIZER_ACCESS_IMPL(Engine::Scene,
                       _ARTAG(_BSTSUPER(Renderable))
                       _ARTAG(m_main_camera_local_id_)
                       _ARTAG(m_layers))

namespace Engine
{
	void Scene::Initialize()
	{
		for(int i = 0; i < LAYER_MAX; ++i)
		{
			m_layers.emplace(static_cast<eLayerType>(i), Instantiate<Layer>(static_cast<eLayerType>(i)));
		}

		const auto camera = Instantiate<Objects::Camera>();
		AddGameObject(camera, LAYER_CAMERA);

		m_mainCamera_ = camera;
		m_main_camera_local_id_ = camera->GetLocalID();

		const auto light1 = Instantiate<Objects::Light>();
		AddGameObject(light1, LAYER_LIGHT);
		light1->SetPosition(Vector3(5.0f, 5.0f, 5.0f));

		const auto light2 = Instantiate<Objects::Light>();
		light2->SetPosition(Vector3(-5.0f, 5.0f, -5.0f));
		light2->SetColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
		AddGameObject(light2, LAYER_LIGHT);

		Initialize_INTERNAL();

#ifdef _DEBUG
		for (const auto& comps : m_cached_components_ | std::views::values)
		{
			if (comps.empty())
			{
				continue;
			}

			if (const auto sample = comps.begin()->lock())
			{
				const auto cast_check = boost::dynamic_pointer_cast<Abstract::IStateController>(sample);

				if (!cast_check) 
				{
					continue;
				}

				for (const auto& comp : comps)
				{
					if (const auto comp_ptr = comp.lock())
					{
						comp_ptr->SetActive(false);
					}
				}
			}
		}

		const auto observer = Instantiate<Objects::Observer>();
		m_observer_ = observer;
		AddGameObject(observer, LAYER_UI);
		GetMainCamera().lock()->BindObject(m_observer_);
#endif
	}

	EntityID Scene::AddGameObject(const StrongObject& obj, eLayerType layer)
	{
		m_layers[layer]->AddGameObject(obj);
		m_cached_objects_.emplace(obj->GetID(), obj);

		for (const auto& comp : obj->GetAllComponents())
		{
			m_cached_components_[comp.lock()->ToTypeName()].emplace(comp);
		}

		obj->GetSharedPtr<Abstract::Actor>()->SetScene(GetSharedPtr<Scene>());
		obj->GetSharedPtr<Abstract::Actor>()->SetLayer(layer);

		ActorID id = 0;

		while (true)
		{
			if (id == g_invalid_id)
			{
				throw std::exception("Actor ID overflow");
			}

			if (!m_assigned_actor_ids_.contains(id))
			{
				m_assigned_actor_ids_.emplace(id);
				break;
			}

			++id;
		}

		obj->GetSharedPtr<Abstract::Actor>()->SetLocalID(id);

		if (const auto tr = obj->GetComponent<Component::Transform>().lock())
		{
			UpdatePosition(obj);
		}

		return obj->GetID();
	}

	void Scene::RemoveGameObject(const EntityID id, eLayerType layer)
	{
		const auto obj = m_cached_objects_[id];

		if (const auto locked = obj.lock())
		{
			const auto tr = locked->GetComponent<Component::Transform>().lock();
			bool updated = false;

			if (tr)
			{
				const auto prev_pos = tr->GetPreviousPosition() + Vector3::One * g_octree_negative_round_up;
				const auto pos = tr->GetPosition() + Vector3::One * g_octree_negative_round_up;

				auto& prev_pos_set = m_object_position_tree_(static_cast<int>(prev_pos.x), static_cast<int>(prev_pos.y), static_cast<int>(prev_pos.z));
				auto& pos_set = m_object_position_tree_(static_cast<int>(pos.x), static_cast<int>(pos.y), static_cast<int>(pos.z));

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

		for (const auto& comp : obj.lock()->GetAllComponents())
		{
			m_cached_components_[comp.lock()->ToTypeName()].erase(comp);
		}

		m_cached_objects_.erase(id);
		m_layers[layer]->RemoveGameObject(id);
		m_assigned_actor_ids_.erase(obj.lock()->GetLocalID());
	}

	std::vector<WeakObject> Scene::GetGameObjects(eLayerType layer)
	{
		return m_layers[layer]->GetGameObjects();
	}

	void Scene::AddCacheComponent(const WeakComponent& component)
	{
		if (m_cached_objects_.contains(component.lock()->GetOwner().lock()->GetID()))
		{
			m_cached_components_[component.lock()->ToTypeName()].insert(component);
		}
	}

	void Scene::RemoveCacheComponent(const WeakComponent& component)
	{
		if (m_cached_objects_.contains(component.lock()->GetOwner().lock()->GetID()))
		{
			m_cached_components_[component.lock()->ToTypeName()].erase(component);
		}
	}

	void Scene::UpdatePosition(const WeakObject& obj)
	{
		if (const auto obj_ptr = obj.lock())
		{
			const auto tr = obj_ptr->GetComponent<Component::Transform>().lock();

			if (!tr)
			{
				GetDebugger().Log(L"Object has no transform component");
				return;
			}

			const auto prev_pos = VectorElementAdd(tr->GetPreviousPosition(), g_octree_negative_round_up);
			const auto pos = VectorElementAdd(tr->GetPosition(), g_octree_negative_round_up);

			if (!VectorElementInRange(prev_pos, g_max_map_size) || !VectorElementInRange(pos, g_max_map_size))
			{
				GetDebugger().Log(L"Object position is out of range");
				return;
			}

			const auto delta = prev_pos - pos;

			if (std::fabsf(delta.x) <= g_epsilon && 
				std::fabsf(delta.y) <= g_epsilon &&
				std::fabsf(delta.z) <= g_epsilon)
			{
				return;
			}

			auto& prev_pos_set = m_object_position_tree_(static_cast<int>(prev_pos.x), static_cast<int>(prev_pos.y), static_cast<int>(prev_pos.z));
			auto& pos_set = m_object_position_tree_(static_cast<int>(pos.x), static_cast<int>(pos.y), static_cast<int>(pos.z));

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

	void Scene::GetNearestObjects(const Vector3& pos, std::vector<WeakObject>& out)
	{
		const auto pos_rounded = VectorElementAdd(pos, g_octree_negative_round_up);

		if (!VectorElementInRange(pos_rounded, g_max_map_size))
		{
			GetDebugger().Log(L"Position is out of range");
			return;
		}

		const auto& pos_set = m_object_position_tree_(static_cast<int>(pos_rounded.x), static_cast<int>(pos_rounded.y), static_cast<int>(pos_rounded.z));

		for (const auto& obj : pos_set)
		{
			if (const auto obj_ptr = obj.lock())
			{
				out.push_back(obj_ptr);
			}
		}
	}

	void Scene::GetNearbyObjects(const Vector3& pos, const size_t range, std::vector<WeakObject>& out)
	{
		const auto pos_rounded = VectorElementAdd(pos, g_octree_negative_round_up);

		for (auto i = static_cast<size_t>(pos_rounded.x) - range; i < static_cast<size_t>(pos_rounded.x) + range; ++i)
		{
			for (auto j = static_cast<size_t>(pos_rounded.y) - range; j < static_cast<size_t>(pos_rounded.y) + range; ++j)
			{
				for (auto k = static_cast<size_t>(pos_rounded.z) - range; k < static_cast<size_t>(pos_rounded.z) + range; ++k)
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

	void Scene::SearchObjects(const Vector3& pos, const Vector3& dir, std::set<WeakObject, WeakObjComparer>& out, int exhaust)
	{
		auto pos_rounded = VectorElementAdd(pos, g_octree_negative_round_up);
		float accumulated_length = 0.f;

		while (static_cast<int>(accumulated_length) < exhaust)
		{
			const auto& current_tree = m_object_position_tree_(static_cast<int>(pos_rounded.x), static_cast<int>(pos_rounded.y), static_cast<int>(pos_rounded.z));

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

	Scene::Scene() : m_main_camera_local_id_(g_invalid_id), m_object_position_tree_(g_max_map_size, {})
	{
	}

	void Scene::Synchronize(const StrongScene& scene)
	{
		GetTaskScheduler().AddTask([this, scene](const float& dt)
		{
			GetDebugger().Log(L"Scene synchronization started.");

			m_layers = scene->m_layers;

			m_main_camera_local_id_ = scene->m_main_camera_local_id_;
			m_mainCamera_ = scene->m_mainCamera_;
			m_cached_objects_ = scene->m_cached_objects_;
			m_cached_components_ = scene->m_cached_components_;
			m_object_position_tree_ = scene->m_object_position_tree_;
			m_assigned_actor_ids_ = scene->m_assigned_actor_ids_;
		});
	}

	void Scene::OpenLoadPopup(bool& is_load_open)
	{
		if (is_load_open)
		{
			if (ImGui::Begin("Load Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
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

	void Scene::AddCustomObject()
	{
	}

	void Scene::PreUpdate(const float& dt)
	{
		m_layers[LAYER_LIGHT]->PreUpdate(dt);

		for (int i = g_early_update_layer_end; i < LAYER_MAX; ++i)
		{
			m_layers[static_cast<eLayerType>(i)]->PreUpdate(dt);
		}
	}

	void Scene::Update(const float& dt)
	{
		m_layers[LAYER_LIGHT]->Update(dt);

		for (int i = g_early_update_layer_end; i < LAYER_MAX; ++i)
		{
			m_layers[static_cast<eLayerType>(i)]->Update(dt);
		}
	}

	void Scene::PreRender(const float dt)
	{
		m_layers[LAYER_LIGHT]->PreRender(dt);

		for (int i = g_early_update_layer_end; i < LAYER_MAX; ++i)
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

	void Scene::Render(const float dt)
	{
		GetRenderPipeline().BindLightBuffers();

		m_layers[LAYER_LIGHT]->Render(dt);

		for (int i = g_early_update_layer_end; i < LAYER_MAX; ++i)
		{
			m_layers[static_cast<eLayerType>(i)]->Render(dt);
		}

#ifdef _DEBUG
		static bool is_load_open = false;

		if (ImGui::Begin(ToTypeName().c_str()), nullptr, ImGuiWindowFlags_MenuBar)
		{
			if (ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("Add"))
				{
					if (ImGui::MenuItem("Camera"))
					{
						const auto camera = Instantiate<Objects::Camera>();
						AddGameObject(camera, LAYER_CAMERA);
					}

					if (ImGui::MenuItem("Light"))
					{
						const auto light = Instantiate<Objects::Light>();
						AddGameObject(light, LAYER_LIGHT);
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
						const auto unique_name = obj_ptr->GetName() + " " + obj_ptr->ToTypeName()+ " " + std::to_string(obj_ptr->GetID());

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
		m_layers[LAYER_LIGHT]->FixedUpdate(dt);

		for (int i = g_early_update_layer_end; i < LAYER_MAX; ++i)
		{
			m_layers[static_cast<eLayerType>(i)]->FixedUpdate(dt);
		}
	}

	void Scene::OnDeserialized()
	{
		Renderable::OnDeserialized();

		auto& ui = m_layers[LAYER_UI]->GetGameObjects();

		// remove observer of previous scene
		for (int i = 0; i < ui.size(); ++i)
		{
			if (boost::dynamic_pointer_cast<Objects::Observer>(ui[i].lock()))
			{
				m_layers[LAYER_UI]->RemoveGameObject(ui[i].lock()->GetID());
				i--;
			}
		}

		// rebuild cache
		for (int i = 0; i < LAYER_MAX; ++i)
		{
			m_layers[static_cast<eLayerType>(i)]->OnDeserialized();

			for (const auto& obj : m_layers[static_cast<eLayerType>(i)]->GetGameObjects())
			{
				m_cached_objects_.emplace(obj.lock()->GetID(), obj);
				obj.lock()->SetScene(GetSharedPtr<Scene>());
				obj.lock()->SetLayer(static_cast<eLayerType>(i));
				m_assigned_actor_ids_.emplace(obj.lock()->GetLocalID());

				for (const auto& comp : obj.lock()->GetAllComponents())
				{
					m_cached_components_[comp.lock()->ToTypeName()].emplace(comp);
				}
			}
		}

		const auto& cameras = m_layers[LAYER_CAMERA]->GetGameObjects();

		const auto it = std::ranges::find_if(cameras, [this](const auto& obj)
		{
			if (obj.lock()->GetLocalID() == m_main_camera_local_id_)
			{
				return true;
			}

			return false;
		});

		if (it != cameras.end())
			m_mainCamera_ = it->lock()->GetSharedPtr<Engine::Objects::Camera>();
		else
			m_mainCamera_ = cameras.begin()->lock()->GetSharedPtr<Engine::Objects::Camera>();

		// @todo: rebuild octree.

#ifdef _DEBUG
		// remove controller if it is debug state
		for (const auto& comps : m_cached_components_ | std::views::values)
		{
			if (comps.empty())
			{
				continue;
			}

			if (const auto sample = comps.begin()->lock())
			{
				const auto cast_check = boost::dynamic_pointer_cast<Abstract::IStateController>(sample);

				if (!cast_check)
				{
					continue;
				}

				for (const auto& comp : comps)
				{
					if (const auto comp_ptr = comp.lock())
					{
						comp_ptr->SetActive(false);
					}
				}
			}
		}

		const auto observer = Instantiate<Objects::Observer>();
		m_observer_ = observer;
		AddGameObject(observer, LAYER_UI);
		GetMainCamera().lock()->BindObject(m_observer_);
#endif
	}
}
