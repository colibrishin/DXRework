#ifdef PHYSX_ENABLED
#include <PxPhysics.h>
#include <PxSceneDesc.h>
#include <PxScene.h>
#include <extensions/PxDefaultSimulationFilterShader.h>
#endif
#include "../Public/Scene.h"
#include "Scene.generated.h"

#include "UIInterface.h"

#include "Source/Runtime/Core/Layer/Public/Layer.h"
#include "Source/Runtime/Core/TaskScheduler/Public/TaskScheduler.h"
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.h"
#include "Source/Runtime/Core/Objects/Camera/Public/Camera.h"
#include "Source/Runtime/Core/Objects/Light/Public/Light.h"
#include "Source/Runtime/Core/Components/Transform/Public/Transform.h"
#include "Components/Collider/Public/Collider.h"

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
	    initializeImpl();
	}

	const bool(&Scene::GetCollisionMask() const)[RESERVED_LAYER_MAX + CFG_LAYER_COUNT][RESERVED_LAYER_MAX + CFG_LAYER_COUNT]
	{
		return m_collision_mask_;
	}

	void Scene::UpdateCollisionMask(const bool collision_mask[RESERVED_LAYER_MAX + CFG_LAYER_COUNT][RESERVED_LAYER_MAX + CFG_LAYER_COUNT])
	{
		memcpy(m_collision_mask_[0], collision_mask[0], sizeof(m_collision_mask_));
	}

    void Scene::initializeForce()
	{
	    Renderable::Initialize();
	}

    Strong<Scene> Scene::cloneImpl()
	{
        auto copy_scene = Strong<Scene>(new Scene());
	    copy_scene->initializeForce();
	    copy_scene->deepCopy( GetSharedPtr<Scene>() );
	    copy_scene->SetName( GetName() + "_Clone" );
	    return copy_scene;
	}

    void Scene::initializeImpl()
	{
	    for (int i = 0; i < RESERVED_LAYER_MAX + CFG_LAYER_COUNT; ++i)
	    {
	        m_layers_.emplace_back(boost::make_shared<Layer>(i));

	        if (i < std::size(g_reserved_layer_name))
	        {
	            m_layers_[i]->SetName(g_reserved_layer_name[i]);
	        }
	    }

	    for (int i = 0; i < size(); ++i)
	    {
	        for (int j = 0; j < size(); ++j)
	        {
	            if (i == j) 
	            {
	                m_collision_mask_[i][j] = true;
	                m_collision_mask_[j][i] = true;
	            }
	            else 
	            {
	                m_collision_mask_[i][j] = false;
	                m_collision_mask_[j][i] = false;
	            }
	        }
	    }
	    
	    const auto& camera       = CreateGameObject<Objects::Camera>(RESERVED_LAYER_CAMERA).lock();
	    m_main_camera_           = camera;
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

			if (!m_assigned_actor_ids_.contains(id))
			{
				m_assigned_actor_ids_.emplace(id, obj->GetID());
				break;
			}

			++id;
		}

		obj->GetSharedPtr<Abstracts::Actor>()->SetLocalID(id);
	}

	void Scene::addGameObjectImpl(const LayerSizeType layer, const Strong<Abstracts::ObjectBase>& obj, bool assign_local_id)
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

		if ( assign_local_id )
		{
            AssignLocalIDToObject( obj );
		}

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
		if (layer == RESERVED_LAYER_LIGHT && obj->GetObjectType() != DEF_OBJ_T_LIGHT)
		{
			throw std::logic_error("Only light object can be added to light layer");
		}
		if (layer == RESERVED_LAYER_CAMERA && obj->GetObjectType() != DEF_OBJ_T_CAMERA)
		{
			throw std::logic_error("Only camera object can be added to camera layer");
		}
		if (layer != RESERVED_LAYER_OBSERVER && obj->GetObjectType() == DEF_OBJ_T_OBSERVER)
		{
			throw std::logic_error("Observer object can only be added to UI layer");
		}

		// add object to scene
		m_layers_[layer]->AddGameObject(obj);
		m_cached_objects_.emplace(obj->GetID(), obj);
		m_concurrent_cached_objects_.emplace(obj->GetID(), obj);

		onObjectAdded.Broadcast(obj);
	}

	void Scene::RemoveObjectFinalize(const GlobalEntityID id, const LayerSizeType layer)
	{
		if (!m_cached_objects_.contains(id))
		{
			// This is not intended to happen.
			throw std::runtime_error("object removal is called twice.");
		}

		const Weak<Abstracts::ObjectBase> obj = m_cached_objects_[id];
		onObjectRemoved.Broadcast(obj);
		
		{
			SpinLockToken token = SingletonSpinLock::GetInstance().Lock(m_component_lock_);
			for (const auto& comp : obj.lock()->GetAllComponents())
			{
				ConcurrentWeakComRootMap::accessor comp_acc;
				if (m_concurrent_cached_components_.find(comp_acc, comp.lock()->GetTypeHash()))
				{
					comp_acc->second.erase(comp.lock()->GetID());
				}
				if (m_cached_components_.contains(comp.lock()->GetTypeHash()))
				{
					m_cached_components_[comp.lock()->GetTypeHash()].erase(comp.lock()->GetID());
				}

				if (comp.lock()->GetTypeHash()->IsDerivedOf(Components::Transform::StaticTypeHash()))
				{
					m_object_position_tree_.Remove(obj.lock());
				}

				if (comp.lock()->GetTypeHash()->IsDerivedOf(Components::Collider::StaticTypeHash()))
				{
					m_object_collision_tree_.Remove(obj.lock());
				}
			}
		}

		obj.lock()->SetScene({});

		if (obj.lock()->GetLocalID() == m_main_actor_local_id_)
		{
			m_main_actor_local_id_ = g_invalid_id;
			m_main_actor_          = {};
		}

		m_cached_objects_.erase(id);
		m_concurrent_cached_objects_.erase(id);
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

			SpinLockToken ot = SingletonSpinLock::GetInstance().Lock(scene->m_object_lock_);
			SpinLockToken ct = SingletonSpinLock::GetInstance().Lock(scene->m_component_lock_);

			m_main_camera_local_id_ = scene->m_main_camera_local_id_;
			m_layers_               = scene->m_layers_;
			m_main_actor_local_id_  = scene->m_main_actor_local_id_;

			m_object_position_tree_.Clear();
			m_object_collision_tree_.Clear();
			m_cached_objects_.clear();
			m_cached_components_.clear();
			m_concurrent_cached_components_.clear();
			m_concurrent_cached_objects_.clear();
			m_assigned_actor_ids_.clear();

			for (const auto& layer : m_layers_)
			{
				for (const auto& obj : layer->GetGameObjects())
				{
					if (const auto locked = obj.lock())
					{
						onObjectAdded.Broadcast(obj);

						m_cached_objects_.emplace(locked->GetID(), locked);
						m_concurrent_cached_objects_.emplace(locked->GetID(), locked);

						m_assigned_actor_ids_.emplace
								(
								 locked->GetLocalID(),
								 locked->GetID()
								);

						if (m_main_actor_local_id_ != g_invalid_id && locked->GetLocalID() == m_main_actor_local_id_)
						{
							m_main_actor_ = locked;
						}

						if (m_main_camera_local_id_ != g_invalid_id && locked->GetLocalID() == m_main_camera_local_id_)
						{
							m_main_camera_ = locked->GetSharedPtr<Objects::Camera>();
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

		    std::memcpy(m_collision_mask_, scene->GetCollisionMask(), sizeof(m_collision_mask_));

			m_object_position_tree_.Update();
			m_object_collision_tree_.Update();

			if (s_debug_observer_)
			{
				DisableControllers();
				AddObserver();
			}
		}
    }

    void Scene::deepCopy( const Weak<Scene> &other )
    {
        if ( const auto scene = other.lock() )
        {
#ifdef PHYSX_ENABLED
            CleanupPhysX();
            InitializePhysX();
#endif

            SpinLockToken ot = SingletonSpinLock::GetInstance().Lock( scene->m_object_lock_ );
            SpinLockToken ct = SingletonSpinLock::GetInstance().Lock( scene->m_component_lock_ );

            m_main_camera_local_id_ = scene->m_main_camera_local_id_;
            m_main_actor_local_id_  = scene->m_main_actor_local_id_;

            m_object_position_tree_.Clear();
            m_object_collision_tree_.Clear();
            m_cached_objects_.clear();
            m_cached_components_.clear();
            m_concurrent_cached_components_.clear();
            m_concurrent_cached_objects_.clear();

            m_assigned_actor_ids_ = scene->m_assigned_actor_ids_;

			UINT idx = 0;
            for ( const auto &layer : scene->m_layers_)
            {
				m_layers_.push_back( boost::make_shared<Layer>( idx ) );
				m_layers_.back()->SetName( layer->GetName() );

                for ( const auto &obj : layer->GetGameObjects() )
                {
                    if ( const auto locked = obj.lock() )
                    {
                        const Strong<Abstracts::ObjectBase> &clone = locked->Clone(false);

                        clone->GetSharedPtr<Abstracts::Actor>()->SetScene( GetSharedPtr<Scene>() );
                        clone->GetSharedPtr<Abstracts::Actor>()->SetLayer( locked->GetLayer() );
                        
						addGameObjectImpl( clone->GetLayer(), clone, false );
                        // Use the same local id as the previous scene. Scene is different so that local id would not
                        // duplicate.
                        clone->GetSharedPtr<Abstracts::Actor>()->SetLocalID( locked->GetLocalID() );

						if ( m_main_actor_local_id_ != g_invalid_id && clone->GetLocalID() == m_main_actor_local_id_ )
                        {
                            m_main_actor_ = clone;
                        }
                        if ( m_main_camera_local_id_ != g_invalid_id && clone->GetLocalID() == m_main_camera_local_id_ )
                        {
                            m_main_camera_ = clone->GetSharedPtr<Objects::Camera>();
                        }
                    }
                }
            }

            std::memcpy( m_collision_mask_, scene->GetCollisionMask(), sizeof( m_collision_mask_ ) );

            m_object_position_tree_.Update();
            m_object_collision_tree_.Update();

            if ( s_debug_observer_ )
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
			SpinLockToken ot = SingletonSpinLock::GetInstance().Lock(m_object_lock_);

			if (!m_cached_objects_.contains(id))
			{
				return;
			}
			if (!m_layers_[layer]->FindGameObject(id).lock())
			{
				return;
			}

			// This object is already flagged to be deleted.
			if (m_cached_objects_[id].lock()->IsGarbage())
			{
				return;
			}

			m_cached_objects_[id].lock()->SetGarbage(true);
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

	Weak<Abstracts::ObjectBase> Scene::FindGameObject(GlobalEntityID id)
	{
		if (id == g_invalid_id)
		{
			return {};
		}

		{
			SpinLockToken ot = SingletonSpinLock::GetInstance().Lock(m_object_lock_);
			if (m_cached_objects_.contains(id))
			{
				return m_cached_objects_.at(id);
			}
		}

		const auto& it = std::find_if
				(
				 m_layers_.begin(), m_layers_.end(),
				 [id](const auto& layer)
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

	Weak<Abstracts::ObjectBase> Scene::FindGameObjectByLocalID(LocalActorID id)
	{
		if (id == g_invalid_id)
		{
			return {};
		}

		if (m_assigned_actor_ids_.contains(id))
		{
			SpinLockToken ot = SingletonSpinLock::GetInstance().Lock(m_object_lock_);
			if (m_cached_objects_.contains(m_assigned_actor_ids_.at(id)))
			{
				return m_cached_objects_.at(m_assigned_actor_ids_.at(id));
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

	void Scene::addCacheComponentImpl(const Strong<Abstracts::Component>& component, const ComponentType type)
	{
		if (!component->GetOwner().lock())
		{
			return;
		}
		
		SpinLockToken ot = SingletonSpinLock::GetInstance().Lock(m_object_lock_);
		SpinLockToken ct = SingletonSpinLock::GetInstance().Lock(m_component_lock_);

		if (m_cached_objects_.contains(component->GetOwner().lock()->GetID()))
		{
			m_cached_components_[type].emplace(component->GetID(), component);

			ConcurrentWeakComRootMap::accessor comp_acc;
			if (!m_concurrent_cached_components_.find(comp_acc, type)) m_concurrent_cached_components_.insert(comp_acc, type);
			comp_acc->second.emplace(component->GetID(), component);

			if (type->IsDerivedOf(Components::Transform::StaticTypeHash()))
			{
				m_object_position_tree_.Insert(component->GetOwner().lock());
			}

			if (type->IsDerivedOf(Components::Collider::StaticTypeHash()))
			{
				m_object_collision_tree_.Insert(component->GetOwner().lock());
			}
		}
	}

	void Scene::removeCacheComponentImpl(const Strong<Abstracts::Component>& component, const ComponentType type)
	{
		SpinLockToken ot = SingletonSpinLock::GetInstance().Lock(m_object_lock_);
		SpinLockToken ct = SingletonSpinLock::GetInstance().Lock(m_component_lock_);

		if (m_cached_objects_.contains(component->GetOwner().lock()->GetID()))
		{
			if (m_cached_components_.contains(type))
			{
				m_cached_components_[type].erase(component->GetID());
				ConcurrentWeakComRootMap::accessor comp_acc;

				if (m_concurrent_cached_components_.find(comp_acc, type)) comp_acc->second.erase(component->GetID());
			}

			if (type->IsDerivedOf(Components::Transform::StaticTypeHash()))
			{
				m_object_position_tree_.Remove(component->GetOwner().lock());
			}

			if (type->IsDerivedOf(Components::Collider::StaticTypeHash()))
			{
				m_object_collision_tree_.Remove(component->GetOwner().lock());
			}
		}
	}

	Scene::Scene() :
	m_b_scene_raytracing_(false),
#ifdef PHYSX_ENABLED
	m_physics_scene_(nullptr),
#endif
	m_main_camera_local_id_(g_invalid_id),
	m_main_actor_local_id_(g_invalid_id),
	m_object_lock_(SingletonSpinLock::GetInstance().Register()),
	m_component_lock_(SingletonSpinLock::GetInstance().Register()){}

	void Scene::BeginPlay( const float dt )
    {
		for (const auto& layer : m_layers_)
		{
            layer->BeginPlay( dt );
		}
    }

    void Scene::EndPlay( const float dt )
    {
        for ( const auto &layer : m_layers_ )
        {
            layer->EndPlay( dt );
        }
	}

    void Scene::PreUpdate( const float dt )
	{
		for (const auto& layer : m_layers_)
		{
			layer->PreUpdate(dt);
		}
	}

	void Scene::Update(const float dt)
	{
		for (const auto& layer : m_layers_)
		{
			layer->Update(dt);
		}

		m_object_position_tree_.Update();
	}

	void Scene::PreRender(const float dt)
	{
		for (const auto& layer : m_layers_)
		{
			layer->PreRender(dt);
		}
	}

	void Scene::Render(const float dt)
	{
		for (const auto& layer : m_layers_)
		{
			layer->Render(dt);
		}
	}

	void Scene::FixedUpdate(const float dt)
	{
		for (const auto& layer : m_layers_)
		{
			layer->FixedUpdate(dt);
		}
	}

	void Scene::PostRender(const float dt)
	{
		for (const auto& layer : m_layers_)
		{
			layer->PostRender(dt);
		}
	}

	void Scene::PostUpdate(const float dt)
	{
		for (const auto& layer : m_layers_)
		{
			layer->PostUpdate(dt);
		}
	}

	void Scene::OnUIUpdate(UIContext* const parent, const float dt)
	{
#if WITH_EDITOR
		UIInterface& ui = UIInterfaceAccessor::GetInterface();

		if (UIContext context = UIInterface::NewContext(ui.NewDialog({this, m_ui_info_.label, m_ui_info_.dialogOpened})))
		{
			context << [&]()
			{
				m_layer_list_box_name_ = "Layers##LayerListBox" + std::to_string(GetID());
				Renderable::OnUIUpdate(&context, dt);
				context += ui.NewListBox({m_layer_list_box_name_, 0, 300.f});

				for (const auto& layer : m_layers_)
				{
					layer->OnUIUpdate(&context, dt);
				}
			};
		}
#endif
	}

	void Scene::OnSerialized()
	{
		Renderable::OnSerialized();

		for (const auto& layer : m_layers_)
		{
			layer->OnSerialized();
		}
	}

	ConcurrentWeakObjVec Scene::GetGameObjectsConcurrent(const LayerSizeType layer) const
	{
		if (layer > m_layers_.size())
		{
			return {};
		}

		return m_layers_[layer]->GetGameObjectsConcurrent();
	}

	WeakObjVec Scene::GetGameObjects(const LayerSizeType layer) const
	{
		auto token = SingletonSpinLock::GetInstance().Lock(m_object_lock_);
		if (layer > m_layers_.size())
		{
			return {};
		}

		return m_layers_[layer]->GetGameObjects();
	}

	Weak<Objects::Camera> Scene::GetMainCamera() const
	{
		return m_main_camera_;
	}

	const Octree& Scene::GetObjectTree()
	{
		return m_object_position_tree_;
	}

	const Octree& Scene::GetCollisionTree()
	{
		return m_object_collision_tree_;
	}

	void Scene::AddObserver()
	{
#if WITH_DEBUG
		DisableControllers();

		const auto& observers = m_layers_[RESERVED_LAYER_OBSERVER]->GetGameObjects();
		for (const auto& observer : observers)
		{
			if (const auto& locked = observer.lock())
			{
				RemoveGameObject(locked->GetID(), RESERVED_LAYER_OBSERVER);
			}
		}

		const auto& observer = CreateGameObject<Objects::Observer>(RESERVED_LAYER_OBSERVER).lock();
		m_observer_         = observer;
		observer->AddChild(GetMainCamera());
#endif
	}

	void Scene::OnDeserialized()
	{
		Renderable::OnDeserialized();

		auto ui = m_layers_[static_cast<size_t>(RESERVED_LAYER_OBSERVER)]->GetGameObjects();

		// remove observer of previous scene
		for (int i = 0; i < ui.size(); ++i)
		{
			if (const auto locked = ui[i].lock())
			{
				if (locked->GetObjectType() == DEF_OBJ_T_OBSERVER)
				{
					m_layers_[static_cast<size_t>(RESERVED_LAYER_OBSERVER)]->RemoveGameObject(ui[i].lock()->GetID());
					i--;
				}
			}
		}

		{
			SpinLockToken ot = SingletonSpinLock::GetInstance().Lock(m_object_lock_);
			SpinLockToken ct = SingletonSpinLock::GetInstance().Lock(m_component_lock_);

			// rebuild cache
			for (int i = 0; i < size(); ++i)
			{
				m_layers_[i]->OnDeserialized();

				for (const auto& obj : m_layers_[i]->GetGameObjects())
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
						m_cached_components_[comp.lock()->GetTypeHash()].emplace(comp.lock()->GetID(), comp);

						if (ConcurrentWeakComRootMap::accessor acc;
							m_concurrent_cached_components_.find(acc, comp.lock()->GetTypeHash()))
						{
							acc->second.emplace(comp.lock()->GetID(), comp);
						}
						else
						{
							m_concurrent_cached_components_.insert(acc, comp.lock()->GetTypeHash());
							acc->second.emplace(comp.lock()->GetID(), comp);
						}
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
			m_main_camera_ = it->lock()->GetSharedPtr<Objects::Camera>();
		}
		else
		{
			m_main_camera_           = cameras.begin()->lock()->GetSharedPtr<Objects::Camera>();
			m_main_camera_local_id_ = cameras.begin()->lock()->GetLocalID();
		}

		// rebuild octree
		m_object_position_tree_.Clear();
		m_object_collision_tree_.Clear();

		for (const auto& object : m_cached_objects_ | std::views::values)
		{
			if (const auto locked = object.lock())
			{
				if (locked->GetComponent<Components::Transform>().lock())
				{
					m_object_position_tree_.Insert(locked);
				}
				if (locked->GetComponent<Components::Collider>().lock())
				{
					m_object_collision_tree_.Insert(locked);
				}
			}
		}

		m_object_position_tree_.Update();
		m_object_collision_tree_.Update();

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
		/*
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
		*/
	}
} // namespace Engine
