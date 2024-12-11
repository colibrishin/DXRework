#ifdef PHYSX_ENABLED
#include <PxPhysics.h>
#include <PxSceneDesc.h>
#include <PxScene.h>
#include <extensions/PxDefaultSimulationFilterShader.h>
#endif
#include "../Public/Scene.hpp"

#include "Source/Runtime/Core/Layer/Public/Layer.h"
#include "Source/Runtime/Core/TaskScheduler/Public/TaskScheduler.h"
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.hpp"
#include "Source/Runtime/Core/Objects/Camera/Public/Camera.h"
#include "Source/Runtime/Core/Objects/Light/Public/Light.h"
#include "Source/Runtime/Core/Components/Transform/Public/Transform.h"

std::atomic<bool> Engine::Scene::s_debug_observer_ = false;

namespace Engine
{
#ifdef PHYSX_ENABLED
	void Scene::InitializePhysX()
	{
		physx::PxSceneDesc scene_desc(GetPhysicsManager().GetPhysX()->getTolerancesScale());
		scene_desc.gravity            = {g_gravity_vec.x, g_gravity_vec.y, g_gravity_vec.z};
		scene_desc.cudaContextManager = GetPhysicsManager().GetCudaContext();
		scene_desc.cpuDispatcher      = GetPhysicsManager().GetCPUDispatcher();
		scene_desc.flags |= physx::PxSceneFlag::eENABLE_GPU_DYNAMICS;
		scene_desc.flags |= physx::PxSceneFlag::eENABLE_BODY_ACCELERATIONS;
		scene_desc.filterShader            = Engine::Physics::SimulationFilterShader;
		scene_desc.filterCallback          = &Engine::Physics::g_filter_callback;
		scene_desc.simulationEventCallback = &Engine::Physics::g_simulation_callback;
		scene_desc.kineKineFilteringMode   = physx::PxPairFilteringMode::eSUPPRESS;
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
	}
#endif

	void Scene::Initialize()
	{
		// won't initialize if already initialized
		if (IsInitialized())
		{
			return;
		}

		Renderable::Initialize();

		for (int i = 0; i < m_layer_count_; ++i)
		{
			m_layers_.emplace_back(boost::make_shared<Layer>(i));
		}

		const auto& camera       = CreateGameObject<Objects::Camera>(RESERVED_LAYER_CAMERA).lock();
		m_mainCamera_           = camera;
		m_main_camera_local_id_ = camera->GetLocalID();

		const auto& light1 = CreateGameObject<Objects::Light>(RESERVED_LAYER_LIGHT).lock();
		light1->GetComponent<Components::Transform>().lock()->SetLocalPosition(Vector3(5.f, 2.f, 5.f));

		const auto& light2 = CreateGameObject<Objects::Light>(RESERVED_LAYER_LIGHT).lock();
		light2->GetComponent<Components::Transform>().lock()->SetLocalPosition(Vector3(-5.f, 2.f, 5.f));

		Managers::TaskScheduler::GetInstance().AddTask
				(
				 TASK_INIT_SCENE,
				 {GetSharedPtr<Scene>()},
				 [](const std::vector<std::any>& params, const float)
				 {
					 const auto& scene = std::any_cast<Strong<Scene>>(params[0]);

					 scene->initializeFinalize();
				 }
				);

#ifdef PHYSX_ENABLED
		InitializePhysX();
#endif
	}

	void Scene::AssignLocalIDToObject(const Strong<Abstracts::ObjectBase>& obj)
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

		obj->GetSharedPtr<Abstracts::Actor>()->SetLocalID(id);
	}

	void Scene::addGameObjectImpl(LayerSizeType layer, const Strong<Abstracts::ObjectBase>& obj)
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
		obj->GetSharedPtr<Abstracts::Actor>()->SetScene(GetSharedPtr<Scene>());
		obj->GetSharedPtr<Abstracts::Actor>()->SetLayer(layer);
		AssignLocalIDToObject(obj);

		if (!obj->IsInitialized())
		{
			obj->Initialize();
		}

		// finalize the object registration at the next frame
		Managers::TaskScheduler::GetInstance().AddTask
				(
				 TASK_ADD_OBJ,
				 {GetSharedPtr<Scene>(), obj, layer}, // keep the object alive, scene does not own the object yet.
				 [](const std::vector<std::any>& params, const float dt)
				 {
					 const auto& scene = std::any_cast<Strong<Scene>>(params[0]);
					 const auto& obj   = std::any_cast<Strong<Abstracts::ObjectBase>>(params[1]);
					 const auto& layer = std::any_cast<LayerSizeType>(params[2]);

					 scene->AddObjectFinalize(layer, obj);
				 }
				);
	}

	void Scene::AddObjectFinalize(const LayerSizeType layer, const Strong<Abstracts::ObjectBase>& obj)
	{
		// add object to scene
		m_layers_[layer]->AddGameObject(obj);
		m_cached_objects_.emplace(obj->GetID(), obj);

		if (layer == RESERVED_LAYER_LIGHT && obj->GetObjectType() != DEF_OBJ_T_LIGHT)
		{
			throw std::logic_error("Only light object can be added to light layer");
		}
		if (layer == RESERVED_LAYER_CAMERA && obj->GetObjectType() != DEF_OBJ_T_CAMERA)
		{
			throw std::logic_error("Only camera object can be added to camera layer");
		}
		if (layer != RESERVED_LAYER_UI && obj->GetObjectType() == DEF_OBJ_T_OBSERVER)
		{
			throw std::logic_error("Observer object can only be added to UI layer");
		}

		onObjectAdded.Broadcast(obj);
	}

	void Scene::RemoveObjectFinalize(const GlobalEntityID id, const LayerSizeType layer)
	{
		Weak<Abstracts::ObjectBase> obj;

		{
			ConcurrentWeakObjGlobalMap::const_accessor acc;

			if (!m_cached_objects_.find(acc, id))
			{
				// This is not intended to happen.
				throw std::runtime_error("object removal is called twice.");
			}

			onObjectRemoved.Broadcast(obj);
			obj = acc->second;
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
		m_layers_[layer]->RemoveGameObject(id);
	}

	void Scene::initializeFinalize()
	{
		AddObserver();
	}

	void Scene::synchronize(const Weak<Scene>& ptr_scene)
	{
		if (const auto scene = ptr_scene.lock())
		{
#ifdef PHYSX_ENABLED
			CleanupPhysX();
			InitializePhysX();
#endif

			m_b_scene_imgui_open_   = scene->m_b_scene_imgui_open_;
			m_main_camera_local_id_ = scene->m_main_camera_local_id_;
			m_layers_               = scene->m_layers_;
			m_mainCamera_           = scene->m_mainCamera_;
			m_main_actor_local_id_  = scene->m_main_actor_local_id_;

			m_object_position_tree_.Clear();
			m_cached_objects_.clear();
			m_cached_components_.clear();
			m_object_position_tree_.Clear();
			m_assigned_actor_ids_.clear();

			for (const auto& layer : m_layers_)
			{
				for (const auto& obj : layer->GetGameObjects())
				{
					if (const auto locked = obj.lock())
					{
						onObjectAdded.Broadcast(obj);

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

			if (s_debug_observer_)
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

	void Scene::ChangeLayer(const LayerSizeType to, const GlobalEntityID id)
	{
		if (const auto& obj = FindGameObject(id).lock())
		{
			if (obj->GetLayer() == to)
			{
				return;
			}

			Managers::TaskScheduler::GetInstance().AddTask
					(
					 TASK_CHANGE_LAYER,
					 {GetSharedPtr<Scene>(), obj->GetSharedPtr<Abstracts::ObjectBase>(), to},
					 [this](const std::vector<std::any>& args, const float)
					 {
						 const auto scene = std::any_cast<Strong<Scene>>(args[0]);
						 const auto obj   = std::any_cast<Strong<Abstracts::ObjectBase>>(args[1]);
						 const auto layer = std::any_cast<LayerSizeType>(args[2]);

						 (*scene)[obj->GetLayer()]->RemoveGameObject(obj->GetID());
						 (*scene)[layer]->AddGameObject(obj);
						 obj->SetLayer(layer);
					 }
					);
		}
	}

	void Scene::RemoveGameObject(const GlobalEntityID id, LayerSizeType layer)
	{
		{
			ConcurrentWeakObjGlobalMap::const_accessor acc;

			if (!m_cached_objects_.find(acc, id))
			{
				return;
			}
			if (!m_layers_[layer]->FindGameObject(id).lock())
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

		Managers::TaskScheduler::GetInstance().AddTask
				(
				 TASK_REM_OBJ,
				 {GetSharedPtr<Scene>()},
				 [id, layer](const std::vector<std::any>& param, const float)
				 {
					 const auto& scene = std::any_cast<Strong<Scene>>(param[0]);

					 scene->RemoveObjectFinalize(id, layer);
				 }
				);
	}

	Weak<Abstracts::ObjectBase> Scene::FindGameObject(GlobalEntityID id) const
	{
		if (id == g_invalid_id)
		{
			return {};
		}

		ConcurrentWeakObjGlobalMap::const_accessor acc;

		if (m_cached_objects_.find(acc, id))
		{
			return acc->second;
		}

		const auto& it = std::find_if
				(
				 m_layers_.begin(), m_layers_.end(),
				 [id, &acc](const auto& layer)
				 {
					 return layer->FindGameObject(id).lock();
				 }
				);

		if (it != m_layers_.end())
		{
			return (*it)->FindGameObject(id);
		}

		return {};
	}

	Weak<Abstracts::ObjectBase> Scene::FindGameObjectByLocalID(LocalActorID id) const
	{
		if (id == g_invalid_id)
		{
			return {};
		}

		ConcurrentLocalGlobalIDMap::const_accessor actor_acc;

		if (m_assigned_actor_ids_.find(actor_acc, id))
		{
			ConcurrentWeakObjGlobalMap::const_accessor acc;

			if (m_cached_objects_.find(acc, actor_acc->second))
			{
				return acc->second;
			}
		}

		for (const auto& layer : m_layers_)
		{
			if (const auto obj = layer->FindGameObjectByLocalID(id).lock())
			{
				return obj;
			}
		}

		return {};
	}

	void Scene::addCacheComponentImpl(const Strong<Abstracts::Component>& component, const eComponentType type)
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

	void Scene::removeCacheComponentImpl(const Strong<Abstracts::Component>& component, const eComponentType type)
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

	void Scene::addCacheScriptImpl(const Strong<Script>& script, const ScriptSizeType type)
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

	void Scene::removeCacheScriptImpl(const Strong<Script>& component, const ScriptSizeType type)
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
#ifdef PHYSX_ENABLED
		  m_physics_scene_(nullptr),
#endif
		  m_main_camera_local_id_(g_invalid_id),
		  m_main_actor_local_id_(g_invalid_id),
		  m_object_position_tree_() {}

	void Scene::PreUpdate(const float& dt)
	{
		for (const auto& layer : m_layers_)
		{
			layer->PreUpdate(dt);
		}
	}

	void Scene::Update(const float& dt)
	{
		for (const auto& layer : m_layers_)
		{
			layer->Update(dt);
		}

		m_object_position_tree_.Update();
	}

	void Scene::PreRender(const float& dt)
	{
		for (const auto& layer : m_layers_)
		{
			layer->PreRender(dt);
		}
	}

	void Scene::Render(const float& dt)
	{
		for (const auto& layer : m_layers_)
		{
			layer->Render(dt);
		}
	}

	void Scene::FixedUpdate(const float& dt)
	{
		for (const auto& layer : m_layers_)
		{
			layer->FixedUpdate(dt);
		}
	}

	void Scene::PostRender(const float& dt)
	{
		for (const auto& layer : m_layers_)
		{
			layer->PostRender(dt);
		}
	}

	void Scene::PostUpdate(const float& dt)
	{
		for (const auto& layer : m_layers_)
		{
			layer->PostUpdate(dt);
		}
	}

	void Scene::OnSerialized()
	{
		Renderable::OnSerialized();

		for (const auto& layer : m_layers_)
		{
			layer->OnSerialized();
		}
	}

	ConcurrentWeakObjVec Scene::GetGameObjects(const LayerSizeType layer) const
	{
		if (layer > m_layers_.size())
		{
			return {};
		}

		return m_layers_[layer]->GetGameObjects();
	}

	Weak<Objects::Camera> Scene::GetMainCamera() const
	{
		return m_mainCamera_;
	}

	const Octree<Weak<Abstracts::ObjectBase>, bounding_getter>& Scene::GetObjectTree()
	{
		return m_object_position_tree_;
	}

	void Scene::AddObserver()
	{
#if WITH_DEBUG
		DisableControllers();
		const auto& observer = CreateGameObject<Objects::Observer>(RESERVED_LAYER_UI).lock();
		m_observer_         = observer;
		observer->AddChild(GetMainCamera());
#endif
	}

	void Scene::OnDeserialized()
	{
		Renderable::OnDeserialized();

		auto ui = m_layers_[static_cast<size_t>(RESERVED_LAYER_UI)]->GetGameObjects();

		// remove observer of previous scene
		for (int i = 0; i < ui.size(); ++i)
		{
			if (const auto locked = ui[i].lock())
			{
				if (locked->GetObjectType() == DEF_OBJ_T_OBSERVER)
				{
					m_layers_[static_cast<size_t>(RESERVED_LAYER_UI)]->RemoveGameObject(ui[i].lock()->GetID());
					i--;
				}
			}
		}

		// rebuild cache
		for (int i = 0; i < m_layer_count_; ++i)
		{
			m_layers_[i]->OnDeserialized();

			for (const auto& obj :
			     m_layers_[i]->GetGameObjects())
			{
				m_cached_objects_.emplace(obj.lock()->GetID(), obj);
				obj.lock()->SetScene(GetSharedPtr<Scene>());
				obj.lock()->SetLayer(i);
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

		for (const auto& layer : m_layers_)
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
		const auto& cameras = m_layers_[RESERVED_LAYER_CAMERA]->GetGameObjects();
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

#if CFG_RAYTRACING
		if (m_b_scene_raytracing_ && !g_raytracing)
		{
			if (!GetRaytracingPipeline().IsRaytracingSupported())
			{
				m_b_scene_raytracing_ = false;
			}
			else
			{
				Managers::TaskScheduler::GetInstance().AddTask
				(
					TASK_TOGGLE_RASTER,
					{ GetSharedPtr<Scene>(), m_b_scene_raytracing_ },
					[](const std::vector<std::any>& params, const float)
					{
						const auto& scene = std::any_cast<Strong<Scene>>(params[0]);
						const auto& b_raytracing = std::any_cast<bool>(params[1]);

						g_raytracing = b_raytracing;
					}
				);
			}
		}
#endif

		AddObserver();
	}

	void Scene::SetMainActor(const LocalActorID id)
	{
		if (const auto& obj = FindGameObjectByLocalID(id).lock())
		{
			m_main_actor_local_id_ = id;
			m_main_actor_          = obj;
		}
	}

	Weak<Abstracts::ObjectBase> Scene::GetMainActor() const
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
