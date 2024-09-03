#include "pch.h"
#include "egScene.hpp"

#ifdef PHYSX_ENABLED
#include <PxPhysics.h>
#include <PxSceneDesc.h>
#include <extensions/PxDefaultSimulationFilterShader.h>
#endif

#include <PxScene.h>

#include "egCamera.h"
#include "egImGuiHeler.hpp"
#include "egLight.h"
#include "egManagerHelper.hpp"
#include "egObserver.h"
#include "PhysXSimulationCallback.h"

SERIALIZE_IMPL
(
	Engine::Scene,
	_ARTAG(_BSTSUPER(Renderable))
	_ARTAG(m_b_scene_raytracing_)
	_ARTAG(m_main_camera_local_id_)
	_ARTAG(m_main_actor_local_id_)
	_ARTAG(m_layers)
)

namespace Engine
{
	void Scene::Initialize()
	{
		// won't initialize if already initialized
		if (IsInitialized())
		{
			return;
		}

		Renderable::Initialize();

		for (int i = 0; i < LAYER_MAX; ++i)
		{
			m_layers.emplace_back(boost::make_shared<Layer>(static_cast<eLayerType>(i)));
		}

		const auto camera       = CreateGameObject<Objects::Camera>(LAYER_CAMERA).lock();
		m_mainCamera_           = camera;
		m_main_camera_local_id_ = camera->GetLocalID();

		const auto light1 = CreateGameObject<Objects::Light>(LAYER_LIGHT).lock();
		light1->GetComponent<Components::Transform>().lock()->SetLocalPosition(Vector3(5.f, 2.f, 5.f));

		const auto light2 = CreateGameObject<Objects::Light>(LAYER_LIGHT).lock();
		light2->GetComponent<Components::Transform>().lock()->SetLocalPosition(Vector3(-5.f, 2.f, 5.f));

		GetTaskScheduler().AddTask
				(
				 TASK_INIT_SCENE,
				 {GetSharedPtr<Scene>()},
				 [](const std::vector<std::any>& params, const float)
				 {
					 const auto& scene = std::any_cast<StrongScene>(params[0]);

					 scene->initializeFinalize();
				 }
				);

#ifdef PHYSX_ENABLED
		physx::PxSceneDesc scene_desc(GetPhysicsManager().GetPhysX()->getTolerancesScale());
		scene_desc.gravity = {g_gravity_vec.x, g_gravity_vec.y, g_gravity_vec.z};
		scene_desc.cudaContextManager = GetPhysicsManager().GetCudaContext();
		scene_desc.cpuDispatcher = GetPhysicsManager().GetCPUDispatcher();
		scene_desc.flags |= physx::PxSceneFlag::eENABLE_GPU_DYNAMICS;
		scene_desc.flags |= physx::PxSceneFlag::eENABLE_BODY_ACCELERATIONS;
		scene_desc.filterShader = Engine::Physics::SimulationFilterShader;
		scene_desc.filterCallback = &Engine::Physics::g_filter_callback;
		scene_desc.simulationEventCallback = &Engine::Physics::g_simulation_callback;
		scene_desc.kineKineFilteringMode = physx::PxPairFilteringMode::eSUPPRESS;
		scene_desc.staticKineFilteringMode = physx::PxPairFilteringMode::eKILL;

		if constexpr (g_speculation_enabled)
		{
			scene_desc.flags |= physx::PxSceneFlag::eENABLE_CCD;
		}

		scene_desc.broadPhaseType = physx::PxBroadPhaseType::eGPU;

		m_physics_scene_ = GetPhysicsManager().GetPhysX()->createScene(scene_desc);

		/*
		 * for the note using PxDefaultSimulationFilterShader
		// runOverlapFilters -> filterShader -> filterRbCollisionPairSecondStage -> mFilterCallback
		physx::PxGroupsMask all_ok;
		std::memset(&all_ok.bits0, std::numeric_limits<uint16_t>::max(), sizeof(uint16_t) * 4);
		physx::PxSetFilterConstants(all_ok, all_ok);
		physx::PxSetFilterBool(true);
		physx::PxSetFilterOps(physx::PxFilterOp::PX_FILTEROP_AND, physx::PxFilterOp::PX_FILTEROP_AND, physx::PxFilterOp::PX_FILTEROP_AND);
		*/
		
		m_physics_scene_->userData = this;
#endif
	}

	void Scene::AssignLocalIDToObject(const StrongObjectBase& obj)
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

	void Scene::addGameObjectImpl(eLayerType layer, const StrongObjectBase& obj)
	{
		// Disconnect the object from the previous scene and layer, if it exists.
		if (const auto scene = obj->GetScene().lock())
		{
			if (const auto& obj_check = scene->FindGameObject(obj->GetID()).lock())
			{
				scene->RemoveGameObject(obj_check->GetID(), obj_check->GetLayer());
			}
		}

		// Cache the pre-existing components
		for (const auto& comp : obj->GetAllComponents())
		{
			if (const auto& locked = comp.lock())
			{
				AddCacheComponent(locked);
			}
		}

		// Set internal information as this scene and layer
		obj->GetSharedPtr<Abstract::Actor>()->SetScene(GetSharedPtr<Scene>());
		obj->GetSharedPtr<Abstract::Actor>()->SetLayer(layer);
		AssignLocalIDToObject(obj);

		if (!obj->IsInitialized())
		{
			obj->Initialize();
		}

		// finalize the object registration at the next frame
		GetTaskScheduler().AddTask
				(
				 TASK_ADD_OBJ,
				 {GetSharedPtr<Scene>(), obj, layer}, // keep the object alive, scene does not own the object yet.
				 [](const std::vector<std::any>& params, const float dt)
				 {
					 const auto& scene = std::any_cast<StrongScene>(params[0]);
					 const auto& obj   = std::any_cast<StrongObjectBase>(params[1]);
					 const auto& layer = std::any_cast<eLayerType>(params[2]);

					 scene->AddObjectFinalize(layer, obj);
				 }
				);
	}

	void Scene::AddObjectFinalize(const eLayerType layer, const StrongObjectBase& obj)
	{
		// add object to scene
		m_layers[layer]->AddGameObject(obj);
		m_cached_objects_.emplace(obj->GetID(), obj);

		if (layer == LAYER_LIGHT && obj->GetObjectType() != DEF_OBJ_T_LIGHT)
		{
			throw std::logic_error("Only light object can be added to light layer");
		}
		if (layer == LAYER_CAMERA && obj->GetObjectType() != DEF_OBJ_T_CAMERA)
		{
			throw std::logic_error("Only camera object can be added to camera layer");
		}
		if (layer != LAYER_UI && obj->GetObjectType() == DEF_OBJ_T_OBSERVER)
		{
			throw std::logic_error("Observer object can only be added to UI layer");
		}

		if (obj->GetObjectType() == DEF_OBJ_T_LIGHT)
		{
			GetShadowManager().RegisterLight(obj->GetSharedPtr<Objects::Light>());
		}
	}

	void Scene::RemoveObjectFinalize(const GlobalEntityID id, eLayerType layer)
	{
		WeakObjectBase obj;

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
			GetShadowManager().UnregisterLight(obj.lock()->GetSharedPtr<Objects::Light>());
		}

		for (const auto& comp : obj.lock()->GetAllComponents())
		{
			ConcurrentWeakComRootMap::accessor comp_acc;

			if (m_cached_components_.find(comp_acc, comp.lock()->GetComponentType()))
			{
				comp_acc->second.erase(comp.lock()->GetID());
			}

			if (comp.lock()->GetComponentType() == COM_T_TRANSFORM)
			{
				m_object_position_tree_.Remove(obj.lock());
			}
		}

		obj.lock()->SetScene({});

		if (obj.lock()->GetLocalID() == m_main_actor_local_id_)
		{
			m_main_actor_local_id_ = g_invalid_id;
			m_main_actor_          = {};
		}

		m_cached_objects_.erase(id);
		m_assigned_actor_ids_.erase(obj.lock()->GetLocalID());
		m_layers[layer]->RemoveGameObject(id);
	}

	void Scene::initializeFinalize()
	{
		AddObserver();
	}

	void Scene::synchronize(const WeakScene& ptr_scene)
	{
		if (const auto scene = ptr_scene.lock())
		{
#ifdef PHYSX_ENABLED
			CleanupPhysX();
#endif
			for (const auto& light : m_layers[LAYER_LIGHT]->GetGameObjects())
			{
				if (const auto locked = light.lock())
				{
					GetShadowManager().UnregisterLight(locked->GetSharedPtr<Objects::Light>());
				}
			}

			m_b_scene_imgui_open_   = scene->m_b_scene_imgui_open_;
			m_main_camera_local_id_ = scene->m_main_camera_local_id_;
			m_layers                = scene->m_layers;
			m_mainCamera_           = scene->m_mainCamera_;
			m_main_actor_local_id_  = scene->m_main_actor_local_id_;

			for (const auto& light : m_layers[LAYER_LIGHT]->GetGameObjects())
			{
				if (const auto locked = light.lock())
				{
					GetShadowManager().RegisterLight(locked->GetSharedPtr<Objects::Light>());
				}
			}

			m_object_position_tree_.Clear();
			m_cached_objects_.clear();
			m_cached_components_.clear();
			m_object_position_tree_.Clear();
			m_assigned_actor_ids_.clear();

			for (const auto& layer : m_layers)
			{
				for (const auto& obj : layer->GetGameObjects())
				{
					if (const auto locked = obj.lock())
					{
						m_cached_objects_.emplace(locked->GetID(), locked);
						m_assigned_actor_ids_.emplace
								(
								 locked->GetLocalID(),
								 locked->GetID()
								);

						if (locked->GetLocalID() == m_main_actor_local_id_)
						{
							m_main_actor_ = locked;
						}

						if (locked->GetLocalID() == m_main_actor_local_id_)
						{
							m_main_actor_ = locked;
						}

						for (const auto& comp : locked->GetAllComponents())
						{
							if (const auto locked_comp = comp.lock())
							{
								AddCacheComponent(locked_comp);
							}
						}

						const auto& children = locked->m_children_;
						locked->m_children_cache_.clear();

						for (const auto& child_id : children)
						{
							if (const auto child = FindGameObjectByLocalID(child_id).lock())
							{
								locked->m_children_cache_.emplace
										(
										 child->GetLocalID(),
										 child
										);

								m_assigned_actor_ids_.emplace
										(
										 child->GetLocalID(),
										 child->GetID()
										);
							}
						}

						if (const auto parent = FindGameObjectByLocalID(locked->m_parent_id_).lock())
						{
							locked->m_parent_ = parent;
						}
					}
				}
			}

			m_object_position_tree_.Update();

			if (g_debug_observer)
			{
				DisableControllers();
				AddObserver();
			}
		}
	}

#ifdef PHYSX_ENABLED
	physx::PxScene* Scene::GetPhysXScene() const
	{
		return m_physics_scene_;
	}

	void Scene::CleanupPhysX()
	{
		if (m_physics_scene_)
		{
			m_physics_scene_->release();
			m_physics_scene_ = nullptr;
		}
	}
#endif

	void Scene::ChangeLayer(const eLayerType to, const GlobalEntityID id)
	{
		if (const auto& obj = FindGameObject(id).lock())
		{
			if (obj->GetLayer() == to)
			{
				return;
			}

			GetTaskScheduler().AddTask
					(
					 TASK_CHANGE_LAYER,
					 {GetSharedPtr<Scene>(), obj->GetSharedPtr<Abstract::ObjectBase>(), to},
					 [this](const std::vector<std::any>& args, const float)
					 {
						 const auto scene = std::any_cast<StrongScene>(args[0]);
						 const auto obj   = std::any_cast<StrongObjectBase>(args[1]);
						 const auto layer = std::any_cast<eLayerType>(args[2]);

						 (*scene)[obj->GetLayer()]->RemoveGameObject(obj->GetID());
						 (*scene)[layer]->AddGameObject(obj);
						 obj->SetLayer(layer);
					 }
					);
		}
	}

	void Scene::RemoveGameObject(const GlobalEntityID id, eLayerType layer)
	{
		{
			ConcurrentWeakObjGlobalMap::const_accessor acc;

			if (!m_cached_objects_.find(acc, id))
			{
				return;
			}
			if (!m_layers[layer]->FindGameObject(id).lock())
			{
				return;
			}

			// This object is already flagged to be deleted.
			if (acc->second.lock()->IsGarbage())
			{
				return;
			}

			acc->second.lock()->SetGarbage(true);
		}

		if (const auto locked = FindGameObject(id).lock())
		{
			if (const auto parent = locked->GetParent().lock())
			{
				parent->DetachChild(locked->GetLocalID());
			}

			if (locked->GetChildren().size() > 0)
			{
				for (const auto& child : locked->GetChildren())
				{
					RemoveGameObject
							(child.lock()->GetID(), child.lock()->GetLayer());
				}
			}
		}

		GetTaskScheduler().AddTask
				(
				 TASK_REM_OBJ,
				 {GetSharedPtr<Scene>()},
				 [id, layer](const std::vector<std::any>& param, const float)
				 {
					 const auto& scene = std::any_cast<StrongScene>(param[0]);

					 scene->RemoveObjectFinalize(id, layer);
				 }
				);
	}

	WeakObjectBase Scene::FindGameObject(GlobalEntityID id) const
	{
		INVALID_ID_CHECK_WEAK_RETURN(id)

		ConcurrentWeakObjGlobalMap::const_accessor acc;

		if (m_cached_objects_.find(acc, id))
		{
			return acc->second;
		}

		const auto& it = std::find_if
				(
				 m_layers.begin(), m_layers.end(),
				 [id, &acc](const auto& layer)
				 {
					 return layer->FindGameObject(id).lock();
				 }
				);

		if (it != m_layers.end())
		{
			return (*it)->FindGameObject(id);
		}

		return {};
	}

	WeakObjectBase Scene::FindGameObjectByLocalID(LocalActorID id) const
	{
		INVALID_ID_CHECK_WEAK_RETURN(id)

		ConcurrentLocalGlobalIDMap::const_accessor actor_acc;

		if (m_assigned_actor_ids_.find(actor_acc, id))
		{
			ConcurrentWeakObjGlobalMap::const_accessor acc;

			if (m_cached_objects_.find(acc, actor_acc->second))
			{
				return acc->second;
			}
		}

		for (const auto& layer : m_layers)
		{
			if (const auto obj = layer->FindGameObjectByLocalID(id).lock())
			{
				return obj;
			}
		}

		return {};
	}

	void Scene::addCacheComponentImpl(const StrongComponent& component, const eComponentType type)
	{
		if (!component->GetOwner().lock())
		{
			return;
		}

		ConcurrentWeakObjGlobalMap::const_accessor acc;

		if (!m_cached_objects_.find(acc, component->GetOwner().lock()->GetID()))
		{
			return;
		}

		ConcurrentWeakComRootMap::accessor comp_acc;

		if (!m_cached_components_.find(comp_acc, type))
		{
			m_cached_components_.insert(comp_acc, type);
		}

		ConcurrentWeakComMap::const_accessor comp_map_acc;

		if (!comp_acc->second.find(comp_map_acc, component->GetID()))
		{
			comp_acc->second.emplace(component->GetID(), component);

			if (type == COM_T_TRANSFORM)
			{
				m_object_position_tree_.Insert(component->GetOwner().lock());
			}
		}
	}

	void Scene::removeCacheComponentImpl(const StrongComponent& component, const eComponentType type)
	{
		ConcurrentWeakObjGlobalMap::const_accessor acc;

		if (m_cached_objects_.find(acc, component->GetOwner().lock()->GetID()))
		{
			ConcurrentWeakComRootMap::accessor comp_acc;
			if (m_cached_components_.find(comp_acc, type))
			{
				comp_acc->second.erase(component->GetID());
			}
		}

		if (type == COM_T_TRANSFORM)
		{
			m_object_position_tree_.Remove(component->GetOwner().lock());
		}
	}

	void Scene::addCacheScriptImpl(const StrongScript& script, const eScriptType type)
	{
		if (!script->GetOwner().lock())
		{
			return;
		}

		if (ConcurrentWeakObjGlobalMap::const_accessor acc;
			m_cached_objects_.find(acc, script->GetOwner().lock()->GetID()))
		{
			if (ConcurrentWeakScpRootMap::accessor scp_acc;
				m_cached_scripts_.find(scp_acc, type))
			{
				if (ConcurrentWeakScpMap::const_accessor scp_map_acc;
					scp_acc->second.find(scp_map_acc, script->GetID()))
				{
					return;
				}

				scp_acc->second.emplace(script->GetID(), script);
			}
			else
			{
				m_cached_scripts_.insert(scp_acc, type);
				scp_acc->second.emplace(script->GetID(), script);
			}
		}
	}

	void Scene::removeCacheScriptImpl(const StrongScript& component, const eScriptType type)
	{
		ConcurrentWeakObjGlobalMap::const_accessor acc;

		if (m_cached_objects_.find(acc, component->GetOwner().lock()->GetID()))
		{
			ConcurrentWeakScpRootMap::accessor scp_acc;
			if (m_cached_scripts_.find(scp_acc, type))
			{
				scp_acc->second.erase(component->GetID());
			}
		}
	}

	Scene::Scene()
		: m_b_scene_imgui_open_(false),
		  m_b_scene_raytracing_(false),
		  m_main_camera_local_id_(g_invalid_id),
		  m_main_actor_local_id_(g_invalid_id),
		  m_object_position_tree_() {}

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

	void Scene::Render(const float& dt)
	{
		for (int i = LAYER_NONE; i < LAYER_MAX; ++i)
		{
			m_layers[static_cast<eLayerType>(i)]->Render(dt);
		}
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

	void Scene::OnSerialized()
	{
		for (int i = LAYER_NONE; i < LAYER_MAX; ++i)
		{
			m_layers[static_cast<eLayerType>(i)]->OnSerialized();
		}
	}

	void Scene::Save()
	{
		Serializer::Serialize(GetName(), GetSharedPtr<Scene>());
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

	void Scene::AddObserver()
	{
		if constexpr (g_debug)
		{
			// add observer if flagged
			if constexpr (g_debug_observer)
			{
				DisableControllers();
				const auto observer = CreateGameObject<Objects::Observer>(LAYER_UI).lock();
				m_observer_         = observer;
				observer->AddChild(GetMainCamera());
			}
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

				if (m_main_actor_local_id_ == obj.lock()->GetLocalID())
				{
					m_main_actor_ = obj;
				}

				for (const auto& comp : obj.lock()->GetAllComponents())
				{
					if (ConcurrentWeakComRootMap::accessor acc;
						m_cached_components_.find
						(acc, comp.lock()->GetComponentType()))
					{
						acc->second.emplace(comp.lock()->GetID(), comp);
					}
					else
					{
						m_cached_components_.insert(acc, comp.lock()->GetComponentType());
						acc->second.emplace(comp.lock()->GetID(), comp);
					}
				}

				for (const auto& scp : obj.lock()->GetAllScripts())
				{
					if (ConcurrentWeakScpRootMap::accessor acc;
						m_cached_scripts_.find
						(acc, scp.lock()->GetScriptType()))
					{
						acc->second.emplace(scp.lock()->GetID(), scp);
					}
					else
					{
						m_cached_scripts_.insert(acc, scp.lock()->GetScriptType());
						acc->second.emplace(scp.lock()->GetID(), scp);
					}
				}
			}
		}

		for (const auto& layer : m_layers)
		{
			for (const auto& obj : layer->GetGameObjects())
			{
				const auto& children = obj.lock()->m_children_;

				for (const auto& child_id : children)
				{
					if (const auto child = FindGameObjectByLocalID(child_id).lock())
					{
						obj.lock()->m_children_cache_.emplace(child->GetLocalID(), child);
					}
				}

				if (const auto parent = FindGameObjectByLocalID(obj.lock()->m_parent_id_).lock())
				{
					obj.lock()->m_parent_ = parent;
				}
			}
		}

		// set main camera
		const auto& cameras = m_layers[LAYER_CAMERA]->GetGameObjects();
		const auto  it      = std::ranges::find_if
				(
				 cameras, [this](const auto& obj)
				 {
					 if (obj.lock()->GetLocalID() == m_main_camera_local_id_)
					 {
						 return true;
					 }

					 return false;
				 }
				);

		// finding the main camera, if there is no main camera matches the local id, set the first camera as main camera.
		if (it != cameras.end())
		{
			m_mainCamera_ = it->lock()->GetSharedPtr<Objects::Camera>();
		}
		else
		{
			m_mainCamera_           = cameras.begin()->lock()->GetSharedPtr<Objects::Camera>();
			m_main_camera_local_id_ = cameras.begin()->lock()->GetLocalID();
		}

		// rebuild octree
		m_object_position_tree_.Clear();

		for (const auto& object : m_cached_objects_ | std::views::values)
		{
			if (const auto locked = object.lock();
				locked->GetComponent<Components::Transform>().lock())
			{
				m_object_position_tree_.Insert(locked);
			}
		}

		m_object_position_tree_.Update();

		if (m_b_scene_raytracing_ && !g_raytracing)
		{
			if (!GetRaytracingPipeline().IsRaytracingSupported())
			{
				m_b_scene_raytracing_ = false;
			}
			else
			{
				GetTaskScheduler().AddTask
				(
					TASK_TOGGLE_RASTER,
					{ GetSharedPtr<Scene>(), m_b_scene_raytracing_ },
					[](const std::vector<std::any>& params, const float)
					{
						const auto& scene = std::any_cast<StrongScene>(params[0]);
						const auto& b_raytracing = std::any_cast<bool>(params[1]);

						g_raytracing = b_raytracing;
					}
				);
			}
		}

		AddObserver();
	}

	void Scene::OnImGui()
	{
		const auto name      = GetName() + "###" + std::to_string(GetID());
		const auto list_name = "###Layers" + std::to_string(GetID());

		if (ImGui::Begin(name.c_str()), &m_b_scene_imgui_open_)
		{
			Renderable::OnImGui();

			if (ImGui::Checkbox("Raytracing", &m_b_scene_raytracing_))
			{
				if (!GetRaytracingPipeline().IsRaytracingSupported())
				{
					m_b_scene_raytracing_ = false;
				}
				else
				{
					GetTaskScheduler().AddTask
					(
						TASK_TOGGLE_RASTER,
						{ GetSharedPtr<Scene>(), m_b_scene_raytracing_ },
						[](const std::vector<std::any>& params, const float)
						{
							const auto& scene = std::any_cast<StrongScene>(params[0]);
							const auto& b_raytracing = std::any_cast<bool>(params[1]);

							g_raytracing = b_raytracing;
						}
					);
				}
			}

			if (ImGui::BeginListBox(list_name.c_str(), {-1, -1}))
			{
				for (int i = LAYER_NONE; i < LAYER_MAX; ++i)
				{
					if (ImGui::TreeNode(g_layer_type_str[i]))
					{
						for (const auto& obj : GetGameObjects(static_cast<eLayerType>(i)))
						{
							if (const auto obj_ptr = obj.lock())
							{
								ImGui::Unindent(4);

								std::string unique_button_name = "Remove###" + std::to_string(obj_ptr->GetID());

								if (ImGui::Button(unique_button_name.c_str()))
								{
									RemoveGameObject(obj_ptr->GetID(), static_cast<eLayerType>(i));
								}

								ImGui::Indent(4);

								ImGui::SameLine();

								const auto unique_name = obj_ptr->GetName() + " " +
								                         obj_ptr->GetPrettyTypeName() + "##" +
								                         std::to_string(obj_ptr->GetID());

								ImGui::Selectable(unique_name.c_str(), &obj_ptr->GetImGuiOpen());

								if (ImGui::BeginDragDropSource())
								{
									ImGui::SetDragDropPayload("OBJECT", &obj, sizeof(WeakObjectBase));
									ImGui::Text(unique_name.c_str());
									ImGui::EndDragDropSource();
								}

								if (obj_ptr->GetImGuiOpen())
								{
									obj_ptr->OnImGui();
								}
							}
						}

						ImGui::TreePop();
						ImGui::Separator();
					}

					if (ImGui::BeginDragDropTarget())
					{
						if (const auto payload = ImGui::AcceptDragDropPayload("OBJECT"))
						{
							if (const auto& obj = static_cast<WeakObjectBase*>(payload->Data)->lock())
							{
								if (obj->GetLayer() == i)
								{
									continue;
								}

								ChangeLayer(static_cast<eLayerType>(i), obj->GetID());
							}
						}
						ImGui::EndDragDropTarget();
					}
				}
				ImGui::EndListBox();
			}


			ImGui::End();
		}
	}

	void Scene::SetMainActor(const LocalActorID id)
	{
		if (const auto& obj = FindGameObjectByLocalID(id).lock())
		{
			m_main_actor_local_id_ = id;
			m_main_actor_          = obj;
		}
	}

	WeakObjectBase Scene::GetMainActor() const
	{
		return m_main_actor_;
	}

	Scene::~Scene()
	{
#ifdef PHYSX_ENABLED
		CleanupPhysX();
#endif
	}

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
} // namespace Engine
