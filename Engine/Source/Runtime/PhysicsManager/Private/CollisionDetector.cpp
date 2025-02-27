#include "CollisionDetector.h"

#include "Verlet.hpp"

#include "Collider.h"
#include "Rigidbody.h"
#include "Transform.h"

#include "Layer.h"

#include "ObjectBase.h"

#include "Scene.h"

#include "SceneManager.h"

#ifdef PHYSX_ENABLED
#include <PxScene.h>
#include "PhysXSimulationCallback.h"
#include <PxRigidActor.h>
#include <PxShape.h>
#include <extensions/PxDefaultSimulationFilterShader.h>
#endif


namespace Engine::Managers
{
#if WITH_EDITOR
	void CollisionDetector::UpdateLayerNames(Weak<Scene> scene)
	{
		m_layer_name_storage_.clear();
		
		if (const Strong<Scene>& locked = scene.lock())
		{
			for (size_t i = 0; i < locked->size(); ++i)
			{
				for (size_t j = 0; j < locked->size(); ++j)
				{
					m_layer_name_storage_[{i, j}] = std::format("{}\n{}", locked->at(i)->GetName(), locked->at(j)->GetName());
				}
			}
		}
	}
#endif

	void CollisionDetector::Initialize()
	{
		UpdateLayerMask(SceneManager::GetInstance().GetActiveScene());
		SceneManager::GetInstance().onSceneActive.Listen(GetSharedPtr<CollisionDetector>(), &CollisionDetector::UpdateScene);

#ifdef PHYSX_ENABLED
		for (int i = 0; i < LAYER_MAX; ++i)
		{
			for (int j = 0; j < LAYER_MAX; ++j)
			{
				physx::PxSetGroupCollisionFlag(i, j, i == j);
			}
		}
#endif

#if WITH_EDITOR
		UpdateLayerNames(SceneManager::GetInstance().GetActiveScene());
		SceneManager::GetInstance().onSceneActive.Listen(GetSharedPtr<CollisionDetector>(), &CollisionDetector::UpdateLayerNames);
#endif

	}

	void CollisionDetector::Update(const float dt) {}

	void CollisionDetector::PreUpdate(const float dt) {}

	void CollisionDetector::PreRender(const float dt) {}

	void CollisionDetector::Render(const float dt) {}

	void CollisionDetector::PostRender(const float dt) {}

	void CollisionDetector::FixedUpdate(const float dt)
	{
        if ( !SceneManager::GetInstance().IsPlaying() )
        {
            return;
        }
	    
		if (const auto scene = SceneManager::GetInstance().GetActiveScene().lock())
		{
#ifdef PHYSX_ENABLED
			scene->GetPhysXScene()->collide(dt);
			scene->GetPhysXScene()->fetchCollision(true);
#else
			const auto& tree = scene->GetCollisionTree();

			std::stack<const Octree*> stack;
			stack.push(&tree);

            std::vector<Octree::PoolVector> node_objects;
            std::map<const Octree*, bool>   visited;

			while (!stack.empty())
			{
				const auto  node     = stack.top();
				const auto& value    = node->Read();
				const auto& children = node->Next();
				const auto& active   = node->ActiveChildren();

				// Walk back from stack, it can be visited again.
				if (visited.contains(node) && visited[node])
				{
					stack.pop();
					continue;
				}

				// Add children to stack.
				for (int i = 7; i >= 0; --i)
				{
					if (children[i])
					{
						stack.push(children[i]);
					}
				}

				// If it never visited, then check collision.
				if (!visited[node])
				{
					// Self collision check
					for (int i = 0; i < value.size(); ++i)
					{
						const auto& obj = value[i].lock();
						if (!obj)
						{
							continue;
						}
						const auto& cl = obj->GetComponent<Components::Collider>().lock();

						// If object is inactive or collider is inactive, then dispatch exit event.
						if (obj && !obj->GetActive() || (cl && !cl->GetActive()))
						{
							DispatchInactiveExit(value[i]);
						}

						for (int j = i + 1; j < value.size(); ++j)
						{
#if CFG_SPECULATION_ENABLED
							TestSpeculation(value[i], value[j], dt);
#endif
							TestCollision(value[i], value[j]);
						}
					}

					// Collision Check between parent and self
					for (int i = 0; i < node_objects.size(); ++i)
					{
						const auto& parent_compare_set = node_objects[i];

						for (int j = 0; j < value.size(); ++j)
						{
							for (int k = 0; k < parent_compare_set.size(); ++k)
							{
#if CFG_SPECULATION_ENABLED
								TestSpeculation(value[j], parent_compare_set[k], dt);
#endif
								TestCollision(value[j], parent_compare_set[k]);
							}
						}
					}
				}

				// Push back to comparison set.
				node_objects.emplace_back(value);
				// Mark as visited so that it doesn't initiate same collision check again.
				visited[node] = true;

				// terminal node
				if (value.size() <= 1 && active == 0)
				{
					node_objects.pop_back();
					stack.pop();
				}
			}
#endif
		}

#if !defined(PHYSX_ENABLED)
		for (const auto& [lhs, rhs_set] : m_frame_collision_map_)
		{
			m_collision_map_[lhs].insert(rhs_set.begin(), rhs_set.end());

			for (const auto& rhs : rhs_set)
			{
				m_collision_map_[rhs].insert(lhs);
			}
		}

		m_frame_collision_map_.clear();
#endif

		// Remove empty set.
		for (auto it = m_collision_map_.begin(); it != m_collision_map_.end();)
		{
			if (it->second.empty())
			{
				it = m_collision_map_.unsafe_erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	void CollisionDetector::PostUpdate(const float dt) {}

#if WITH_EDITOR
	void CollisionDetector::OnUIUpdate(UIContext* const parent, const float dt)
	{
        IUIAPI &ui = s_uia.GetInterface();
        if ( UIContext context = IUIAPI::NewContext(
                ui.NewDialog( this, "CollisionDetectorDialog", { m_ui_info_.label, m_ui_info_.dialogOpened } ) ) )
        {
            if ( const Strong<Scene> &scene = SceneManager::GetInstance().GetActiveScene().lock() )
            {
                context += ui.NewTable( this, "CollisionDetectorLayerMaskTable", { "Layer Mask", scene->size() } );
                for ( size_t i = 0; i < scene->size(); ++i )
                {
                    context |= ui.NewTableRow( this, std::format( "Row{}", i ), {} );
                    for ( size_t j = 0; j <= i; ++j )
                    {
                        context |= ui.NewTableColumn( this, std::format( "Column{}", j ), {} );
                        ( context |= ui.NewCheckbox( this,
                                                     std::format( "Checkbox{}{}", i, j ),
                                                     { m_layer_name_storage_[ { i, j } ], m_layer_mask_[ i ][ j ], true } ) ).
                                SetFunction( [this]()
                                {
                                    if ( const Strong<Scene> &scene = SceneManager::GetInstance().GetActiveScene().
                                            lock() )
                                    {
                                        scene->UpdateCollisionMask( m_layer_mask_ );
                                    }
                                } );
                    }
				}
				--context;
			}
		}
	}
#endif
	
	void CollisionDetector::TestCollision(const Weak<Abstracts::ObjectBase>& p_lhs, const Weak<Abstracts::ObjectBase>& p_rhs)
	{
		const auto lhs = p_lhs.lock();
		const auto rhs = p_rhs.lock();

		if (!lhs || !rhs)
		{
			return;
		}
		if (!IsCollisionLayer(lhs->GetLayer(), rhs->GetLayer()))
		{
			return;
		}
		if (lhs->GetParent().lock() || rhs->GetParent().lock())
		{
			if (lhs->GetParent().lock() == rhs || rhs->GetParent().lock() == lhs)
			{
				return;
			}
			if (lhs->GetParent().lock() == rhs->GetParent().lock())
			{
				return;
			}
		}
		if (!lhs->GetActive() || !rhs->GetActive())
		{
			return;
		}

		// Octree sanity check
		if (lhs == rhs)
		{
			throw std::logic_error("Self collision detected");
		}

		// Speculation caught.
		if (m_frame_collision_map_.contains(lhs->GetID()) &&
		    m_frame_collision_map_[lhs->GetID()].contains(rhs->GetID()))
		{
			return;
		}

		const auto ltr = lhs->GetComponent<Components::Transform>().lock();
		const auto rtr = rhs->GetComponent<Components::Transform>().lock();

		const auto lcl = lhs->GetComponent<Components::Collider>().lock();
		const auto rcl = rhs->GetComponent<Components::Collider>().lock();

		if (lcl && rcl)
		{
			if (!lcl->GetActive() || !rcl->GetActive())
			{
				return;
			}
			const bool collision = Components::Collider::Intersects(lcl, rcl);

			if (collision)
			{
				if (!m_collision_map_.contains(lhs->GetID()) ||
				    !m_collision_map_[lhs->GetID()].contains(rhs->GetID()))
				{
					// Initial Collision
					m_frame_collision_map_[lhs->GetID()].insert(rhs->GetID());
					m_frame_collision_map_[rhs->GetID()].insert(lhs->GetID());

					lcl->onCollisionEnter.Broadcast(rcl);
					rcl->onCollisionEnter.Broadcast(lcl);
				}

				const auto lrb = lhs->GetComponent<Components::Rigidbody>().lock();
				const auto rrb = rhs->GetComponent<Components::Rigidbody>().lock();

				if (lrb && rrb)
				{
					m_collision_produce_queue_.push_back({lhs, rhs, false, true});
				}

				// Or continuous collision
				lcl->AddCollidedObject(rhs->GetID());
				rcl->AddCollidedObject(lhs->GetID());
			}
			else
			{
				if (m_collision_map_.contains(lhs->GetID()) &&
				    m_collision_map_[lhs->GetID()].contains(rhs->GetID()))
				{
					// Final Collision
					m_collision_map_[lhs->GetID()].erase(rhs->GetID());
					m_collision_map_[rhs->GetID()].erase(lhs->GetID());

					lcl->onCollisionEnd.Broadcast(rcl);
					rcl->onCollisionEnd.Broadcast(lcl);
					lcl->RemoveCollidedObject(rhs->GetID());
					rcl->RemoveCollidedObject(lhs->GetID());
				}

				// No collision
			}
		}
	}

	void CollisionDetector::TestSpeculation(const Weak<Abstracts::ObjectBase>& p_lhs, const Weak<Abstracts::ObjectBase>& p_rhs, const float dt)
	{
		auto lhs = p_lhs.lock();
		auto rhs = p_rhs.lock();

		if (!lhs || !rhs)
		{
			return;
		}
		if (!IsCollisionLayer(lhs->GetLayer(), rhs->GetLayer()))
		{
			return;
		}
		if (lhs->GetParent().lock() || rhs->GetParent().lock())
		{
			if (lhs->GetParent().lock() == rhs || rhs->GetParent().lock() == lhs)
			{
				return;
			}
			if (lhs->GetParent().lock() == rhs->GetParent().lock())
			{
				return;
			}
		}
		if (!lhs->GetActive() || !rhs->GetActive())
		{
			return;
		}

		// Octree sanity check
		if (lhs == rhs)
		{
			throw std::logic_error("Self collision detected");
		}
		if (m_frame_collision_map_.contains(lhs->GetID()) &&
		    m_frame_collision_map_[lhs->GetID()].contains(rhs->GetID()))
		{
			throw std::logic_error("Double check occurred");
		}

		auto ltr = lhs->GetComponent<Components::Transform>().lock();
		auto rtr = rhs->GetComponent<Components::Transform>().lock();

		auto lcl = lhs->GetComponent<Components::Collider>().lock();
		auto rcl = rhs->GetComponent<Components::Collider>().lock();

		// To speculate, the velocity of object is required.
		auto lrb = lhs->GetComponent<Components::Rigidbody>().lock();
		auto rrb = rhs->GetComponent<Components::Rigidbody>().lock();

		// Assuming lhs always has the rigid-body or both have, for moving object backward easily.
		if (!lrb && !rrb)
		{
			return;
		}
		// Move rigid-body object to lhs or fixed object to rhs.
		if ((rrb && !lrb) || (lrb && lrb->IsFixed()))
		{
			std::swap(lhs, rhs);
			std::swap(ltr, rtr);
			std::swap(lcl, rcl);
			std::swap(lrb, rrb);
		}

		// If rhs was not exist, then skip.
		if (!lrb)
		{
			return;
		}

		if (lcl && rcl)
		{
			// If any of object collider is disabled, then skip.
			if (!lcl->GetActive() || !rcl->GetActive())
			{
				return;
			}

			bool collision1 = false;
			bool collision2 = false;

			// lhs test
			const auto lvel = EvalT1PositionDelta(lrb->GetT0LinearVelocity(), lrb->GetT0Force(), dt);
			Vector3    ldir;
			lvel.Normalize(ldir);

			// no need to check if velocity is zero.
			if (ldir != Vector3::Zero || !MathExtension::FloatCompare
			    (lvel.Length(), 0.f) || !lrb->GetActive())
			{
				collision1 = Components::Collider::Intersects
						(lcl, rcl, lvel.Length(), ldir);
			}

			// rhs test, if exists.
			if (rrb && rrb->GetActive())
			{
				const auto rvel = EvalT1PositionDelta
						(rrb->GetT0LinearVelocity(), rrb->GetT0Force(), dt);
				Vector3 rdir;
				rvel.Normalize(rdir);
				if (rdir != Vector3::Zero || !MathExtension::FloatCompare
				    (rvel.Length(), 0.f))
				{
					collision2 = Components::Collider::Intersects(rcl, lcl, rvel.Length(), rdir);
				}
			}

			// If any of collision is true, then it is speculative hit.
			if (collision1 || collision2)
			{
				//GetDebugger().Log(std::format("Speculative hit, {}, {}", lhs->GetName(), rhs->GetName()));

				if (!m_collision_map_.contains(lhs->GetID()) ||
				    !m_collision_map_[lhs->GetID()].contains(rhs->GetID()))
				{
					// Initial Collision
					m_frame_collision_map_[lhs->GetID()].insert(rhs->GetID());
					m_frame_collision_map_[rhs->GetID()].insert(lhs->GetID());

					lcl->onCollisionEnter.Broadcast(rcl);
					rcl->onCollisionEnter.Broadcast(lcl);

					m_collision_produce_queue_.push_back({lhs, rhs, true, true});
				}

				// Or continuous collision
				lcl->onCollisionEnd.Broadcast(rcl);
				rcl->onCollisionEnd.Broadcast(lcl);
				lcl->AddCollidedObject(rhs->GetID());
				rcl->AddCollidedObject(lhs->GetID());
			}
		}
	}

	void CollisionDetector::DispatchInactiveExit(const Weak<Abstracts::ObjectBase>& lhs)
	{
		const auto lcl   = lhs.lock()->GetComponent<Components::Collider>().lock();
		const auto scene = SceneManager::GetInstance().GetActiveScene().lock();

		if (!lcl)
		{
			return;
		}

		const auto& collided = lcl->GetCollidedObjects();

		for (const auto& id : collided)
		{
			if (const auto rhs = scene->FindGameObject(id).lock())
			{
				if (const auto rcl = rhs->GetComponent<Components::Collider>().lock())
				{
					if (rcl->IsCollidedObject(lhs.lock()->GetID()))
					{
						rcl->RemoveCollidedObject(lhs.lock()->GetID());

						m_collision_map_[rhs->GetID()].erase(lhs.lock()->GetID());
						m_collision_map_[lhs.lock()->GetID()].erase(rhs->GetID());

						lcl->onCollisionEnd.Broadcast(rcl);
						rcl->onCollisionEnd.Broadcast(lcl);
					}
				}
			}
		}
	}

#ifdef PHYSX_ENABLED
	uint32_t CollisionDetector::GetLayerFilter(const eLayerType layer) const
	{
		uint32_t filter = 0;

		for (int i = 0; i < LAYER_MAX; ++i)
		{
			if (m_layer_mask_[layer][i])
			{
				filter += 1 << i;
			}
		}

		return filter;
	}
#endif

	void CollisionDetector::SetCollisionLayer(
		const LayerSizeType a,
		const LayerSizeType b
	)
	{
		m_layer_mask_[a][b] = true;
		m_layer_mask_[b][a] = true;

#ifdef PHYSX_ENABLED
		physx::PxSetGroupCollisionFlag(a, b, true);
		physx::PxSetGroupCollisionFlag(b, a, true);
#endif

		onLayerMaskChange.Broadcast(a, b);
	}

	void CollisionDetector::UnsetCollisionLayer(LayerSizeType layer, LayerSizeType layer2)
	{
		m_layer_mask_[layer][layer2] = false;
		m_layer_mask_[layer2][layer] = false;

#ifdef PHYSX_ENABLED
		physx::PxSetGroupCollisionFlag(layer, layer2, false);
		physx::PxSetGroupCollisionFlag(layer2, layer, false);
#endif

		onLayerMaskChange.Broadcast(layer, layer2);
	}

	bool CollisionDetector::IsCollisionLayer(LayerSizeType layer1, LayerSizeType layer2)
	{
		std::lock_guard l(m_layer_mask_mutex_);
		return m_layer_mask_[layer1][layer2];
	}

	bool CollisionDetector::IsCollided(GlobalEntityID id) const
	{
		if (!m_collision_map_.contains(id))
		{
			return false;
		}

		return !m_collision_map_.at(id).empty();
	}

	bool CollisionDetector::IsCollided(GlobalEntityID id1, GlobalEntityID id2) const
	{
		if (!m_collision_map_.contains(id1))
		{
			return false;
		}

		return m_collision_map_.at(id1).contains(id2);
	}

	tbb::concurrent_vector<CollisionInfo>& CollisionDetector::GetCollisionInfo()
	{
		return m_collision_produce_queue_;
	}

	CollisionDetector::~CollisionDetector()
	{
	}

	void CollisionDetector::UpdateLayerMask(const Weak<Scene> scene)
	{
		if (const Strong<Scene>& locked = scene.lock())
		{
			const auto& mask = locked->GetCollisionMask();

			for (int i = 0; i < std::size(mask); ++i) 
			{
				for (int j = 0; j < std::size(mask[0]); ++j)
				{
					m_layer_mask_[i][j] = mask[i][j];
				}
			}
		}
	}

	void CollisionDetector::UpdateScene(const Weak<Scene> scene)
	{
		UpdateLayerMask(scene);
	}

	bool CollisionDetector::IsCollidedInFrame(GlobalEntityID id1, GlobalEntityID id2) const
	{
		if (!m_frame_collision_map_.contains(id1))
		{
			return false;
		}

		return m_frame_collision_map_.at(id1).contains(id2);
	}
} // namespace Engine::Managers