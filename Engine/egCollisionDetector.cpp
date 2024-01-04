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

    void CollisionDetector::CheckCollision(StrongCollider& lhs, StrongCollider& rhs)
    {
        const auto lhs_owner = lhs->GetOwner().lock();
        const auto rhs_owner = rhs->GetOwner().lock();

        if ((!m_speculation_map_[lhs_owner->GetID()].contains(rhs_owner->GetID()) &&
            g_speculation_enabled && CheckRaycasting(lhs, rhs)))
        {
            m_speculation_map_[lhs_owner->GetID()].insert(rhs_owner->GetID());
            m_speculation_map_[rhs_owner->GetID()].insert(lhs_owner->GetID());

            GetDebugger().Log(
                              L"Speculation Hit! : " +
                              std::to_wstring(lhs_owner->GetID()) + L" " +
                              std::to_wstring(rhs_owner->GetID()));

            lhs_owner->DispatchComponentEvent(lhs, rhs);
            rhs_owner->DispatchComponentEvent(rhs, lhs);

            m_collision_produce_queue_.push_back({lhs_owner, rhs_owner, true, true});
            return;
        }

        if (lhs->Intersects(rhs))
        {
            if (m_collision_map_[lhs_owner->GetID()].contains(rhs_owner->GetID()))
            {
                m_collision_map_[lhs_owner->GetID()].insert(rhs_owner->GetID());
                m_collision_map_[rhs_owner->GetID()].insert(lhs_owner->GetID());

                lhs_owner->DispatchComponentEvent(lhs, rhs);
                rhs_owner->DispatchComponentEvent(rhs, lhs);
            }
            else if (!m_frame_collision_map_[lhs_owner->GetID()].contains(rhs_owner->GetID()))
            {
                m_frame_collision_map_[lhs_owner->GetID()].insert(rhs_owner->GetID());
                m_frame_collision_map_[rhs_owner->GetID()].insert(lhs_owner->GetID());

                lhs_owner->DispatchComponentEvent(lhs, rhs);
                rhs_owner->DispatchComponentEvent(rhs, lhs);

                if (const auto lhs_rb = lhs_owner->GetComponent<Components::Rigidbody>().lock();
                    lhs_rb && lhs_rb->IsFixed())
                {
                    m_collision_produce_queue_.push_back({rhs_owner, lhs_owner, false, true});
                }
                else if (const auto rhs_rb = rhs_owner->GetComponent<Components::Rigidbody>().lock();
                    rhs_rb && rhs_rb->IsFixed())
                {
                    m_collision_produce_queue_.push_back({lhs_owner, rhs_owner, false, true});
                }
                else
                {
                    m_collision_produce_queue_.push_back({lhs_owner, rhs_owner, false, true});
                }
            }
            else
            {
                lhs_owner->DispatchComponentEvent(lhs, rhs);
                rhs_owner->DispatchComponentEvent(rhs, lhs);
            }
        }
        else
        {
            if (m_collision_map_[lhs_owner->GetID()].contains(rhs_owner->GetID()))
            {
                m_collision_map_[lhs_owner->GetID()].erase(rhs_owner->GetID());
                m_collision_map_[rhs_owner->GetID()].erase(lhs_owner->GetID());

                lhs_owner->DispatchComponentEvent(lhs, rhs);
                rhs_owner->DispatchComponentEvent(rhs, lhs);
            }
        }
    }

    void CollisionDetector::CheckGrounded(const StrongCollider& lhs, const StrongCollider& rhs)
    {
        const auto lhs_owner = lhs->GetOwner().lock();
        const auto rhs_owner = rhs->GetOwner().lock();

        const auto rb = lhs_owner
                        ->GetComponent<Components::Rigidbody>()
                        .lock();

        if (Components::Collider::Intersects(lhs, rhs, Vector3::Down * g_epsilon))
        {
            // Ground flag is automatically set to false on the start of the frame.
            rb->SetGrounded(true);

            m_collision_produce_queue_.push_back({lhs_owner, rhs_owner, false, true, true});
        }
    }

    bool CollisionDetector::CheckRaycasting(const StrongCollider& lhs, const StrongCollider& rhs)
    {
        const auto rb =
                lhs->GetOwner().lock()->GetComponent<Components::Rigidbody>().lock();

        const auto rb_other =
                rhs->GetOwner().lock()->GetComponent<Components::Rigidbody>().lock();

        if (rb && rb_other)
        {
            Ray ray{};
            const auto tr = rb->GetOwner().lock()->GetComponent<Components::Transform>().lock();
            ray.position        = tr->GetWorldPreviousPosition();
            const auto velocity = rb->GetLinearMomentum();
            velocity.Normalize(ray.direction);

            if (ray.direction == Vector3::Zero) return false;

            const auto length = velocity.Length();
            float      dist;

            return rhs->Intersects(ray, length, dist);
        }

        return false;
    }

    void CollisionDetector::Update(const float& dt)
    {
        const auto  scene     = GetSceneManager().GetActiveScene().lock();

        for (int i = 0; i < LAYER_MAX; ++i)
        {
            for (int j = i; j < LAYER_MAX; ++j)
            {
                {
                    std::lock_guard l(m_layer_mask_mutex_);
                    if (!m_layer_mask_[i].test(j)) continue;
                }

                const auto objects = (*scene)[i]->GetGameObjects();

                for (int k = 0; k < objects.size(); ++k)
                {
                    if (objects[k].expired()) continue;
                    const auto lhs = objects[k].lock();

                    if (!lhs->GetActive()) continue;

                    for (int l = k + 1; l < objects.size(); ++l)
                    {
                        if (objects[l].expired()) continue;
                        const auto rhs = objects[l].lock();

                        if (!rhs->GetActive()) continue;
                        if (lhs->GetParent().lock() == rhs->GetParent().lock()) continue;
                        if (rhs->GetParent().lock() == lhs) continue;
                        if (lhs->GetParent().lock() == rhs) continue;

                        auto lhs_cl = lhs->GetComponent<Components::Collider>().lock();
                        auto rhs_cl = rhs->GetComponent<Components::Collider>().lock();

                        if (!lhs_cl || !rhs_cl) continue;

                        CheckCollision(lhs_cl, rhs_cl);
                    }
                }
            }
        }

        m_collision_check_map_.clear();
    }

    void CollisionDetector::PreUpdate(const float& dt)
    {
        m_collision_map_.merge(m_frame_collision_map_);

        m_frame_collision_map_.clear();
        m_speculation_map_.clear();

        const auto  scene     = GetSceneManager().GetActiveScene().lock();
        const auto  rbs = scene->GetCachedComponents<Components::Rigidbody>();

        for (int i = 0; i < rbs.size(); ++i)
        {
            if (rbs[i].expired()) continue;
            const auto lhs = rbs[i].lock();

            if (lhs->GetSharedPtr<Components::Rigidbody>()->IsFixed()) continue;

            for (int j = 0; j < rbs.size(); ++j)
            {
                if (rbs[j].expired()) continue;
                const auto rhs = rbs[j].lock();

                if (i == j) continue;

                {
                    std::lock_guard lock(m_layer_mask_mutex_);
                    if (!m_layer_mask_[lhs->GetOwner().lock()->GetLayer()].test(
                                                                                rhs->GetOwner().lock()->GetLayer()))
                    {
                        continue;
                    }
                }

                const auto lhs_cl = lhs->GetOwner().lock()->GetComponent<Components::Collider>().lock();
                const auto rhs_cl = rhs->GetOwner().lock()->GetComponent<Components::Collider>().lock();

                CheckGrounded(lhs_cl, rhs_cl);
            }
        }
    }

    void CollisionDetector::PreRender(const float& dt) {}

    void CollisionDetector::Render(const float& dt) {}

    void CollisionDetector::PostRender(const float& dt) {}

    void CollisionDetector::FixedUpdate(const float& dt) {}

    void CollisionDetector::PostUpdate(const float& dt) {}

    void CollisionDetector::SetCollisionLayer(
        const eLayerType a,
        const eLayerType b)
    {
        m_layer_mask_[a].set(b, true);
        m_layer_mask_[b].set(a, true);
    }

    void CollisionDetector::GetCollidedObjects(
        const Ray&                                            ray, const float distance,
        std::set<WeakObject, WeakComparer<Abstract::Object>>& out)
    {
        const auto scene = GetSceneManager().GetActiveScene().lock();

        if (!scene)
        {
            GetDebugger().Log(L"CollisionDetector: Scene has not loaded.");
            out = {};
        }

        std::mutex out_mutex;

        std::for_each(
                      std::execution::par, scene->begin(),
                      scene->end(),
                      [ray, &distance, &out,
                          &out_mutex](const StrongLayer& layer)
                      {
                          const auto objects = layer->GetGameObjects();

                          std::for_each(
                                        std::execution::par, objects.begin(), objects.end(),
                                        [ray, &distance, &out, &out_mutex](const WeakObject& obj)
                                        {
                                            const auto obj_locked = obj.lock();
                                            const auto cl = obj_locked->GetComponent<Components::Collider>().lock();

                                            if (!cl)
                                            {
                                                return;
                                            }

                                            float intersection;

                                            if (cl->Intersects(ray, distance, intersection))
                                            {
                                                {
                                                    std::lock_guard lock(out_mutex);
                                                    out.insert(obj);
                                                }
                                            }
                                        });
                      });
    }

    bool CollisionDetector::Hitscan(
        const Ray&                                            ray, const float distance,
        std::set<WeakObject, WeakComparer<Abstract::Object>>& out)
    {
        std::set<WeakObject, WeakComparer<Abstract::Object>> intermid_out;
        GetSceneManager().GetActiveScene().lock()->SearchObjects(
                                                                 ray.position, ray.direction, intermid_out,
                                                                 static_cast<int>(distance));

        bool hit = false;

        std::mutex out_mutex;

        std::for_each(
                      std::execution::par, intermid_out.begin(), intermid_out.end(),
                      [ray, distance, &hit, &out, &out_mutex](const WeakObject& obj)
                      {
                          if (const auto locked = obj.lock())
                          {
                              const auto cl = locked->GetComponent<Components::Collider>().lock();

                              if (!cl)
                              {
                                  return;
                              }

                              float ground;

                              if (cl->Intersects(ray, distance, ground))
                              {
                                  GetDebugger().Log(
                                                    L"Octree Hit! : " +
                                                    std::to_wstring(locked->GetID()));

                                  {
                                      std::lock_guard lock(out_mutex);
                                      hit |= true;
                                      out.insert(obj);
                                  }
                              }
                          }
                      });

        if (hit)
        {
            return true;
        }

        if (out.empty())
        {
            GetDebugger().Log(L"Octree hits nothing, trying with bruteforce...");

            GetCollisionDetector().GetCollidedObjects(ray, distance, out);

            for (const auto& obj : out)
            {
                if (const auto locked = obj.lock())
                {
                    GetDebugger().Log(
                                      L"Bruteforce Hit! : " +
                                      std::to_wstring(locked->GetID()));
                }
            }

            return true;
        }

        return false;
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

    bool CollisionDetector::IsSpeculated(GlobalEntityID id1, GlobalEntityID id2) const
    {
        if (!m_speculation_map_.contains(id1))
        {
            return false;
        }

        return m_speculation_map_.at(id1).contains(id2);
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
