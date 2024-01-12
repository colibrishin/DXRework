#include "pch.h"

#include "egCollisionDetector.h"
#include "egBaseCollider.hpp"
#include "egCollision.h"
#include "egElastic.h"
#include "egManagerHelper.hpp"
#include "egObject.hpp"
#include "egRigidbody.h"
#include "egSceneManager.hpp"
#include "egTransform.h"

namespace Engine::Manager::Physics
{
    void CollisionDetector::Initialize()
    {
        for (int i = 0; i < LAYER_MAX; ++i)
        {
            for (int j = 0; j < LAYER_MAX; ++j)
            {
                if (i == j)
                {
                    m_layer_mask_[i].set(j, true);
                    m_layer_mask_[j].set(i, true);
                }
                else
                {
                    m_layer_mask_[i].set(j, false);
                    m_layer_mask_[j].set(i, false);
                }
            }
        }
    }

    void CollisionDetector::Update(const float& dt)
    {
        if (const auto scene = GetSceneManager().GetActiveScene().lock())
        {
            const auto& tree = scene->GetObjectTree();

            std::stack<const Octree*> stack;
            stack.push(&tree);

            std::vector<std::vector<WeakObject>>               node_objects;
            std::map<const Octree*, bool>                      visited;

            while (!stack.empty())
            {
                const auto node = stack.top();
                const auto& value = node->Read();
                const auto& children = node->Next();
                const auto& active = node->ActiveChildren();

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
                        for (int j = i + 1; j < value.size(); ++j)
                        {
                            TestCollision(value[i], value[j]);
                        }
                    }

                    // Collision Check between parent and self
                    for (int i = 0; i < node_objects.size(); ++i)
                    {
                        const auto parent_compare_set = node_objects[i];

                        for (int j = 0; j < value.size(); ++j)
                        {
                            for (int k = 0; k < parent_compare_set.size(); ++k)
                            {
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
                    node_objects.erase(node_objects.end() - 1);
                    stack.pop();
                }
            }
        }
    }

    void CollisionDetector::PreUpdate(const float& dt)
    {
        m_collision_map_.merge(m_frame_collision_map_);
        m_frame_collision_map_.clear();

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

    void CollisionDetector::PreRender(const float& dt) {}

    void CollisionDetector::Render(const float& dt) {}

    void CollisionDetector::PostRender(const float& dt) {}

    void CollisionDetector::FixedUpdate(const float& dt) {}

    void CollisionDetector::PostUpdate(const float& dt) {}

    void CollisionDetector::TestCollision(const WeakObject& p_lhs, const WeakObject& p_rhs)
    {
        const auto lhs = p_lhs.lock();
        const auto rhs = p_rhs.lock();

        if (!IsCollsionLayer(lhs->GetLayer(), rhs->GetLayer())) return;

        if (!lhs || !rhs) return;
        if (lhs == rhs) throw std::logic_error("Self collision detected.");
        if (lhs->GetParent().lock() == rhs || rhs->GetParent().lock() == lhs) return;
        if (!lhs->GetActive() || !rhs->GetActive()) return;

        const auto ltr = lhs->GetComponent<Components::Transform>().lock();
        const auto rtr = rhs->GetComponent<Components::Transform>().lock();

        const auto lcl = lhs->GetComponent<Components::Collider>().lock();
        const auto rcl = rhs->GetComponent<Components::Collider>().lock();

        if (lcl && rcl)
        {
            const bool collision = Components::Collider::Intersects(lcl, rcl);

            if (collision)
            {
                if (!m_collision_map_.contains(lhs->GetID()) && 
                    !m_collision_map_[lhs->GetID()].contains(rhs->GetID()))
                {
                    // Initial Collision
                    m_frame_collision_map_[lhs->GetID()].insert(rhs->GetID());
                    m_frame_collision_map_[rhs->GetID()].insert(lhs->GetID());
                }

                m_collision_produce_queue_.push_back({lhs, rhs, false, true});

                // Or continuous collision
                lhs->DispatchComponentEvent(rcl);
                rhs->DispatchComponentEvent(lcl);
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

                    lhs->DispatchComponentEvent(rcl);
                    rhs->DispatchComponentEvent(lcl);
                    lcl->RemoveCollidedObject(rhs->GetID());
                    rcl->RemoveCollidedObject(lhs->GetID());
                }

                // No collision
            }
        }
    }

    void CollisionDetector::SetCollisionLayer(
        const eLayerType a,
        const eLayerType b)
    {
        m_layer_mask_[a].set(b, true);
        m_layer_mask_[b].set(a, true);
    }

    bool CollisionDetector::IsCollsionLayer(eLayerType layer1, eLayerType layer2)
    {
        std::lock_guard l(m_layer_mask_mutex_);
        return m_layer_mask_[layer1].test(layer2);
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

    concurrent_vector<CollisionInfo>& CollisionDetector::GetCollisionInfo()
    {
        return m_collision_produce_queue_;
    }

    bool CollisionDetector::IsCollidedInFrame(GlobalEntityID id1, GlobalEntityID id2) const
    {
        if (!m_frame_collision_map_.contains(id1))
        {
            return false;
        }

        return m_frame_collision_map_.at(id1).contains(id2);
    }
} // namespace Engine::Manager
