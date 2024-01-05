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

    void CollisionDetector::CheckCollisionImpl(const StrongCollider& lhs, const StrongCollider& rhs)
    {
        auto c_lhs = lhs;
        auto c_rhs = rhs;

        auto lhs_owner = lhs->GetOwner().lock();
        auto rhs_owner = rhs->GetOwner().lock();

        // Two objects are already checked and in the collision course.
        if (IsCollidedInFrame(lhs_owner->GetID(), rhs_owner->GetID())) return;

        const auto rb = lhs_owner->GetComponent<Components::Rigidbody>().lock();

        // Assuming that lhs is always the moving object or two objects are static.
        if (rb && rb->IsFixed())
        {
            std::swap(c_lhs, c_rhs);
            std::swap(lhs_owner, rhs_owner);
        }

        if constexpr (g_speculation_enabled && 
                      !IsSpeculated(lhs_owner->GetID(), rhs_owner->GetID()) && 
                      CheckRaycasting(c_lhs, c_rhs))
        {
            GetDebugger().Log(
                              "Speculation Hit! : " +
                              std::to_string(lhs_owner->GetID()) + " " +
                              std::to_string(rhs_owner->GetID()));

            if (!IsCollided(lhs_owner->GetID(), rhs_owner->GetID()))
            {
                InFrameColliding(c_lhs, c_rhs, lhs_owner, rhs_owner);

                c_lhs->AddCollidedObject(rhs_owner->GetID());
                c_rhs->AddCollidedObject(lhs_owner->GetID());
            }

            // Resolving the speculation and after than the collision.
            m_collision_produce_queue_.push_back({lhs_owner, rhs_owner, true, true});

            // In speculation, assumes that collision will eventually happen. Skip the collision check.
            return;
        }

        if (Components::Collider::Intersects(c_lhs, c_rhs))
        {
            if (!IsCollided(lhs_owner->GetID(), rhs_owner->GetID()))
            {
                // This is the first contact.
                const auto lhs_rb = lhs_owner->GetComponent<Components::Rigidbody>().lock();
                const auto rhs_rb = rhs_owner->GetComponent<Components::Rigidbody>().lock();

                // Resolves the collision only in the first frame that collision occurred.
                // Still adds the count for collided, for reducing the energy of the collision.
                // Both are fixed or does not have rigidbody, we do not need to resolve the collision.
                // However, they do need to know that they are collided.
                if (rhs_rb && lhs_rb)
                    m_collision_produce_queue_.push_back({lhs_owner, rhs_owner, false, true});

                InFrameColliding(c_lhs, c_rhs, lhs_owner, rhs_owner);
            }
            else
            {
                // This is the continuous collision.
                ContinuousColliding(c_lhs, c_rhs, lhs_owner, rhs_owner);
            }

            IncreaseCollisionCounter(c_lhs, c_rhs, lhs_owner, rhs_owner);
        }
        else
        {
            // Now no longer in collision course.
            ExitColliding(c_lhs, c_rhs, lhs_owner, rhs_owner);
            RemoveCollisionCounter(c_lhs, c_rhs, lhs_owner, rhs_owner);

            // It is and was not in collision course.
        }
    }

    void CollisionDetector::CheckGrounded(const StrongCollider& lhs, const StrongCollider& rhs)
    {
        auto c_lhs = lhs;
        auto c_rhs = rhs;

        auto lhs_owner = lhs->GetOwner().lock();
        auto rhs_owner = rhs->GetOwner().lock();

        auto rb = lhs_owner
                        ->GetComponent<Components::Rigidbody>()
                        .lock();
        auto rb_other = rhs_owner
                              ->GetComponent<Components::Rigidbody>()
                              .lock();

        if (rb->IsFixed())
        {
            std::swap(c_lhs, c_rhs);
            std::swap(lhs_owner, rhs_owner);
            std::swap(rb, rb_other);
        }

        if (Components::Collider::Intersects(c_lhs, c_rhs, Vector3::Down))
        {
            // Ground flag is automatically set to false on the fixed frame update. (i.e., physics update)
            // pre-update -> collision detection -> ground = true -> fixed update -> physics update (w/o gravity) -> ground = false
            rb->SetGrounded(true);

            if (!IsCollided(c_lhs->GetID(), c_rhs->GetID()))
            {
                InFrameColliding(c_lhs, c_rhs, lhs_owner, rhs_owner);

                // The object hits the "ground" object.
                m_collision_produce_queue_.push_back({lhs_owner, rhs_owner, false, true, true});
            }
            else
            {
                ContinuousColliding(c_lhs, c_rhs, lhs_owner, rhs_owner);
            }

            IncreaseCollisionCounter(c_lhs, c_rhs, lhs_owner, rhs_owner);
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

    void CollisionDetector::CheckCollision(const ConcurrentWeakObjVec& rhsl, const StrongObject& lhs, int idx)
    {
        const auto rhs = rhsl[idx].lock();
        if (!rhs) return;

        if (!rhs->GetActive()) return;
        if (lhs->GetParent().lock() == rhs->GetParent().lock()) return;
        if (rhs->GetParent().lock() == lhs) return;
        if (lhs->GetParent().lock() == rhs) return;

        const auto lhs_cl = lhs->GetComponent<Components::Collider>().lock();
        const auto rhs_cl = rhs->GetComponent<Components::Collider>().lock();

        if (!lhs_cl || !rhs_cl) return;
        if (!lhs_cl->GetActive() || !rhs_cl->GetActive()) return;

        CheckCollisionImpl(lhs_cl, rhs_cl);
        return;
    }

    void CollisionDetector::Update(const float& dt)
    {
        m_speculation_map_.clear();

        const auto  scene     = GetSceneManager().GetActiveScene().lock();

        for (int i = 0; i < LAYER_MAX; ++i)
        {
            for (int j = i; j < LAYER_MAX; ++j)
            {
                if (!CheckLayerCollidable(i, j)) continue;

                const auto lhsl = (*scene)[i]->GetGameObjects();
                const auto rhsl = (*scene)[j]->GetGameObjects();

                if (i == j)
                {
                    PreCheckCollisionSameLayer(scene, lhsl, rhsl);
                    continue;
                }

                for (int k = 0; k < lhsl.size(); ++k)
                {
                    PreCheckCollisionDiffLayer(scene, lhsl, rhsl, k);
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

        const auto  scene     = GetSceneManager().GetActiveScene().lock();
        // Ignore the static object being grounded.
        const auto  rbs = scene->GetCachedComponents<Components::Rigidbody>();

        for (int i = 0; i < rbs.size(); ++i)
        {
            const auto lhs = rbs[i].lock();
            if (!lhs) continue;
            if (lhs->GetSharedPtr<Components::Rigidbody>()->IsFixed()) continue;
            if (!lhs->GetActive() || !lhs->GetOwner().lock()->GetActive()) continue;

            // There could be the case where two objects are moving downwards and colliding with each other.
            // then, first object is considered as grounded and second object is not.
            for (int j = 0; j < rbs.size(); ++j)
            {
                // No need to check ground status with same object.
                if (i == j) continue;
                const auto rhs = rbs[j].lock();
                if (!rhs) continue;
                if (!rhs->GetActive() || !rhs->GetOwner().lock()->GetActive()) continue;

                CheckLayerCollidable(lhs->GetOwner().lock()->GetLayer(), rhs->GetOwner().lock()->GetLayer());

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
            GetDebugger().Log("CollisionDetector: Scene has not loaded.");
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
                                                    "Octree Hit! : " +
                                                    std::to_string(locked->GetID()));

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
            GetDebugger().Log("Octree hits nothing, trying with bruteforce...");

            GetCollisionDetector().GetCollidedObjects(ray, distance, out);

            for (const auto& obj : out)
            {
                if (const auto locked = obj.lock())
                {
                    GetDebugger().Log(
                                      "Bruteforce Hit! : " +
                                      std::to_string(locked->GetID()));
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

    __forceinline void CollisionDetector::ContinuousColliding(
        const StrongCollider& lhs, const StrongCollider& rhs, const StrongObject& lhs_owner,
        const StrongObject&   rhs_owner) const
    {
        if (IsCollided(lhs_owner->GetID(), rhs_owner->GetID()))
        {
            // This is the continuous collision.
            lhs_owner->DispatchComponentEvent(rhs);
            rhs_owner->DispatchComponentEvent(lhs);
        }
    }

    __forceinline void CollisionDetector::InFrameColliding(
        const StrongCollider& lhs, const StrongCollider& rhs, const StrongObject& lhs_owner,
        const StrongObject&   rhs_owner)
    {
        if (!IsCollided(lhs_owner->GetID(), rhs_owner->GetID()))
        {
            m_frame_collision_map_[lhs_owner->GetID()].insert(rhs_owner->GetID());
            m_frame_collision_map_[rhs_owner->GetID()].insert(lhs_owner->GetID());

            lhs_owner->DispatchComponentEvent(rhs);
            rhs_owner->DispatchComponentEvent(lhs);
        }
    }

    __forceinline void CollisionDetector::ExitColliding(
        const StrongCollider& lhs, const StrongCollider& rhs, const StrongObject& lhs_owner,
        const StrongObject&   rhs_owner)
    {
        if (IsCollided(lhs_owner->GetID(), rhs_owner->GetID()))
        {
            m_collision_map_[lhs_owner->GetID()].erase(rhs_owner->GetID());
            m_collision_map_[rhs_owner->GetID()].erase(lhs_owner->GetID());

            lhs_owner->DispatchComponentEvent(rhs);
            rhs_owner->DispatchComponentEvent(lhs);
        }
    }

    __forceinline void CollisionDetector::CheckInactiveCollision(const StrongScene& scene, const StrongObject& lhs)
    {
        if (IsCollided(lhs->GetID()))
        {
            auto& set = m_collision_map_[lhs->GetID()];

            if (set.empty()) return;
            auto it = set.begin();

            while (it != set.end())
            {
                const auto rhs = scene->FindGameObject(*it).lock();
                if (!rhs) 
                {
                    it = set.erase(it);
                    continue;
                }

                const auto lhs_cl = lhs->GetComponent<Components::Collider>().lock();
                const auto rhs_cl = rhs->GetComponent<Components::Collider>().lock();

                ExitColliding(lhs_cl, rhs_cl, lhs, rhs);
                RemoveCollisionCounter(lhs_cl, rhs_cl, lhs, rhs);

                it = set.begin();
            }

            // This will not remove the collision map due to unsafe_erase.
            // remaining empty set will be removed in the merge phase, pre-update.
        }
    }

    __forceinline void CollisionDetector::IncreaseCollisionCounter(
        const StrongCollider& lhs, const StrongCollider& rhs, const StrongObject& lhs_owner,
        const StrongObject&   rhs_owner)
    {
        lhs->AddCollidedObject(rhs_owner->GetID());
        rhs->AddCollidedObject(lhs_owner->GetID());
    }

    __forceinline void CollisionDetector::RemoveCollisionCounter(
        const StrongCollider& lhs, const StrongCollider& rhs, const StrongObject& lhs_owner,
        const StrongObject&   rhs_owner)
    {
        lhs->RemoveCollidedObject(rhs_owner->GetID());
        rhs->RemoveCollidedObject(lhs_owner->GetID());
    }

    __forceinline void CollisionDetector::PreCheckCollisionSameLayer(
        const StrongScene& scene, const ConcurrentWeakObjVec& lhsl, const ConcurrentWeakObjVec& rhsl)
    {
        for (int k = 0; k < lhsl.size(); ++k)
        {
            const auto lhs = lhsl[k].lock();
            if (!lhs) continue;
            if (!lhs->GetActive())
            {
                CheckInactiveCollision(scene, lhs);
                continue;
            }

            for (int l = k + 1; l < rhsl.size(); ++l)
            {
                CheckCollision(rhsl, lhs, l);
            }
        }
    }

    __forceinline void CollisionDetector::PreCheckCollisionDiffLayer(
        const StrongScene& scene, const ConcurrentWeakObjVec& lhsl, const ConcurrentWeakObjVec& rhsl, int idx)
    {
        const auto lhs = lhsl[idx].lock();
        if (!lhs) return;
        if (!lhs->GetActive() || !lhs->GetComponent<Components::Collider>().lock()->GetActive())
        {
            CheckInactiveCollision(scene, lhs);
            return;
        }

        for (int l = 0; l < rhsl.size(); ++l)
        {
            CheckCollision(rhsl, lhs, l);
        }
    }

    __forceinline bool CollisionDetector::CheckLayerCollidable(int i, int j)
    {
        std::lock_guard l(m_layer_mask_mutex_);
        if (!m_layer_mask_[i].test(j)) return false;
        return true;
    }
} // namespace Engine::Manager
