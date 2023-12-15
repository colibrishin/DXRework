#include "pch.h"

#include "egCollisionDetector.h"
#include "egCollider.hpp"
#include "egCollision.h"
#include "egElastic.h"
#include "egManagerHelper.hpp"
#include "egObject.hpp"
#include "egRigidbody.h"
#include "egSceneManager.hpp"

namespace Engine::Manager
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

    void CollisionDetector::CheckCollision(
        Component::Collider& lhs,
        Component::Collider& rhs)
    {
        const auto lhs_owner = lhs.GetOwner().lock();
        const auto rhs_owner = rhs.GetOwner().lock();

        if (CheckRaycasting(lhs, rhs) && g_speculation_enabled)
        {
            if (!m_speculation_map_[lhs_owner->GetID()].contains(rhs_owner->GetID()))
            {
                m_speculation_map_[lhs_owner->GetID()].insert(rhs_owner->GetID());
                m_speculation_map_[rhs_owner->GetID()].insert(lhs_owner->GetID());

                GetDebugger().Log(
                                  L"Speculation Hit! : " +
                                  std::to_wstring(lhs_owner->GetID()) + L" " +
                                  std::to_wstring(rhs_owner->GetID()));

                lhs_owner->DispatchComponentEvent(lhs, rhs);
                rhs_owner->DispatchComponentEvent(rhs, lhs);
                return;
            }
        }

        if (lhs.Intersects(rhs))
        {
            if (m_collision_map_[lhs_owner->GetID()].contains(rhs_owner->GetID()))
            {
                m_collision_map_[lhs_owner->GetID()].insert(rhs_owner->GetID());
                m_collision_map_[rhs_owner->GetID()].insert(lhs_owner->GetID());

                lhs_owner->DispatchComponentEvent(lhs, rhs);
                rhs_owner->DispatchComponentEvent(rhs, lhs);
                return;
            }

            m_frame_collision_map_[lhs_owner->GetID()].insert(rhs_owner->GetID());
            m_frame_collision_map_[rhs_owner->GetID()].insert(lhs_owner->GetID());

            lhs_owner->DispatchComponentEvent(lhs, rhs);
            rhs_owner->DispatchComponentEvent(rhs, lhs);
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

    void CollisionDetector::CheckGrounded(
        const Component::Collider& lhs,
        Component::Collider&       rhs)
    {
        Component::Collider copy = lhs;
        copy.SetPosition(lhs.GetPosition() + Vector3::Down * g_epsilon);

        if (copy.Intersects(rhs))
        {
            if (const auto rb = lhs.GetOwner()
                                   .lock()
                                   ->GetComponent<Component::Rigidbody>()
                                   .lock())
            {
                // Ground flag is automatically set to false on the start of the frame.
                rb->SetGrounded(true);
            }
        }
    }

    bool CollisionDetector::CheckRaycasting(
        const Component::Collider& lhs,
        const Component::Collider& rhs)
    {
        const auto rb =
                lhs.GetOwner().lock()->GetComponent<Component::Rigidbody>().lock();

        const auto rb_other =
                rhs.GetOwner().lock()->GetComponent<Component::Rigidbody>().lock();

        if (rb && rb_other)
        {
            static Ray ray{};
            ray.position        = lhs.GetPreviousPosition();
            const auto velocity = rb->GetLinearMomentum();
            velocity.Normalize(ray.direction);

            const auto length = velocity.Length();
            float      dist;

            return rhs.Intersects(ray, length, dist);
        }

        return false;
    }

    void CollisionDetector::Update(const float& dt)
    {
        const auto  scene     = GetSceneManager().GetActiveScene().lock();
        const auto& colliders = scene->GetCachedComponents<Component::Collider>();

        for (const auto& cl : colliders)
        {
            for (const auto& cl_other : colliders)
            {
                if (cl.lock() == cl_other.lock())
                {
                    continue;
                }

                const auto cl_locked       = cl.lock();
                const auto cl_other_locked = cl_other.lock();

                if (!cl_locked || !cl_other_locked)
                {
                    continue;
                }

                if (!m_layer_mask_[cl_locked->GetOwner().lock()->GetLayer()].test(
                 cl_other_locked->GetOwner().lock()->GetLayer()))
                {
                    continue;
                }

                CheckCollision(
                               *cl_locked->GetSharedPtr<Component::Collider>(),
                               *cl_other_locked->GetSharedPtr<Component::Collider>());
            }
        }
    }

    void CollisionDetector::PreUpdate(const float& dt)
    {
        m_collision_map_.merge(m_frame_collision_map_);

        m_frame_collision_map_.clear();
        m_speculation_map_.clear();

        const auto  scene     = GetSceneManager().GetActiveScene().lock();
        const auto& colliders = scene->GetCachedComponents<Component::Collider>();

        for (const auto& cl : colliders)
        {
            for (const auto& cl_other : colliders)
            {
                if (cl.lock() == cl_other.lock())
                {
                    continue;
                }

                const auto cl_locked       = cl.lock();
                const auto cl_other_locked = cl_other.lock();

                if (!cl_locked || !cl_other_locked)
                {
                    continue;
                }

                if (!m_layer_mask_[cl_locked->GetOwner().lock()->GetLayer()].test(
                 cl_other_locked->GetOwner().lock()->GetLayer()))
                {
                    continue;
                }

                CheckGrounded(
                              *cl_locked->GetSharedPtr<Component::Collider>(),
                              *cl_other_locked->GetSharedPtr<Component::Collider>());
            }
        }
    }

    void CollisionDetector::PreRender(const float& dt) {}

    void CollisionDetector::Render(const float& dt) {}

    void CollisionDetector::PostRender(const float& dt) {}

    void CollisionDetector::FixedUpdate(const float& dt) {}

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
                      std::execution::par, scene->serialized_layer_begin(),
                      scene->serialized_layer_end(),
                      [ray, &distance, &out,
                          &out_mutex](const std::pair<const eLayerType, StrongLayer>& layer)
                      {
                          const auto objects = layer.second->GetGameObjects();

                          std::for_each(
                                        std::execution::par, objects.begin(), objects.end(),
                                        [ray, &distance, &out, &out_mutex](const WeakObject& obj)
                                        {
                                            const auto obj_locked = obj.lock();
                                            const auto cls        = obj_locked->GetComponents<Component::Collider>();

                                            for (const auto& collider : cls)
                                            {
                                                const auto cl = collider.lock();

                                                if (!cl)
                                                {
                                                    continue;
                                                }

                                                float intersection;

                                                if (cl->Intersects(ray, distance, intersection))
                                                {
                                                    {
                                                        std::lock_guard lock(out_mutex);
                                                        out.insert(obj);
                                                    }
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
                              const auto cls = locked->GetComponents<Component::Collider>();

                              std::for_each(
                                            std::execution::par, cls.begin(), cls.end(),
                                            [ray, distance, &hit, &out, &out_mutex, &obj,
                                                locked](const WeakCollider& cl_o)
                                            {
                                                const auto cl = cl_o.lock();

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
                                            });
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

    bool CollisionDetector::IsCollided(EntityID id) const
    {
        if (!m_collision_map_.contains(id))
        {
            return false;
        }

        return !m_collision_map_.at(id).empty();
    }

    bool CollisionDetector::IsCollided(EntityID id1, EntityID id2) const
    {
        if (!m_collision_map_.contains(id1))
        {
            return false;
        }

        return m_collision_map_.at(id1).contains(id2);
    }

    bool CollisionDetector::IsSpeculated(EntityID id1, EntityID id2) const
    {
        if (!m_speculation_map_.contains(id1))
        {
            return false;
        }

        return m_speculation_map_.at(id1).contains(id2);
    }

    bool CollisionDetector::IsCollidedInFrame(EntityID id1, EntityID id2) const
    {
        if (!m_frame_collision_map_.contains(id1))
        {
            return false;
        }

        return m_frame_collision_map_.at(id1).contains(id2);
    }
} // namespace Engine::Manager
