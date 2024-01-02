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

    void CollisionDetector::MarkAsChecked(const StrongObject& lhs_owner, const StrongObject& rhs_owner)
    {
        m_collision_check_map_[lhs_owner->GetID()].insert(rhs_owner->GetID());
        m_collision_check_map_[rhs_owner->GetID()].insert(lhs_owner->GetID());
    }

    void CollisionDetector::CheckCollision(StrongBaseCollider& lhs, StrongBaseCollider& rhs)
    {
        const auto lhs_owner = lhs->GetOwner().lock();
        const auto rhs_owner = rhs->GetOwner().lock();

        if (lhs_owner == rhs_owner) return;
        if (const auto      lhs_parent = lhs_owner->GetParent().lock(); lhs_parent == rhs_owner) return;
        else if (const auto rhs_parent = rhs_owner->GetParent().lock(); rhs_parent == lhs_owner) return;

        if (m_collision_check_map_[lhs_owner->GetID()].contains(rhs_owner->GetID()))
        {
            return;
        }

        if ((!m_speculation_map_[lhs_owner->GetID()].contains(rhs_owner->GetID()) &&
            CheckRaycasting(lhs, rhs) && g_speculation_enabled))
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
            MarkAsChecked(lhs_owner, rhs_owner);
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

        MarkAsChecked(lhs_owner, rhs_owner);
    }

    void CollisionDetector::CheckCollisionChunk(StrongBaseCollider& lhs, std::vector<StrongBaseCollider>& rhs)
    {
        for (auto& rhs_cl : rhs)
        {
            CheckCollision(lhs, rhs_cl);
        }
    }

    void CollisionDetector::CheckGrounded(const StrongBaseCollider& lhs, const StrongBaseCollider& rhs)
    {
        const auto lhs_owner = lhs->GetOwner().lock();
        const auto rhs_owner = rhs->GetOwner().lock();

        if (lhs_owner == rhs_owner) return;
        if (const auto      lhs_parent = lhs_owner->GetParent().lock(); lhs_parent == rhs_owner) return;
        else if (const auto rhs_parent = rhs_owner->GetParent().lock(); rhs_parent == lhs_owner) return;

        if (Components::BaseCollider::Intersects(lhs, rhs, Vector3::Down * g_epsilon))
        {
            if (const auto rb = lhs_owner
                                   ->GetComponent<Components::Rigidbody>()
                                   .lock())
            {
                // Ground flag is automatically set to false on the start of the frame.
                rb->SetGrounded(true);
            }
        }
    }

    bool CollisionDetector::CheckRaycasting(const StrongBaseCollider& lhs, const StrongBaseCollider& rhs)
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

            const auto length = velocity.Length();
            float      dist;

            return rhs->Intersects(ray, length, dist);
        }

        return false;
    }

    void CollisionDetector::Update(const float& dt)
    {
        const auto  scene     = GetSceneManager().GetActiveScene().lock();
        const auto& colliders = scene->GetCachedComponents<Components::BaseCollider>();

        static tbb::affinity_partitioner ap;

        tbb::parallel_for(
                          tbb::blocked_range<size_t>(0, colliders.size()),
                          [colliders, this](const tbb::blocked_range<size_t>& range)
                          {
                              for (size_t i = range.begin(); i != range.end(); ++i)
                              {
                                  auto ptr_lhs = colliders[i].lock();

                                  if (!ptr_lhs)
                                  {
                                      continue;
                                  }

                                  auto lhs = ptr_lhs->GetSharedPtr<Components::BaseCollider>();

                                  std::vector<StrongBaseCollider> rhs_chunk;

                                  for (size_t j = 0; j < colliders.size(); ++j)
                                  {
                                      const auto rhs = colliders[j].lock();

                                      if (!rhs)
                                      {
                                          continue;
                                      }

                                      if (ptr_lhs == rhs)
                                      {
                                          continue;
                                      }

                                      if (ptr_lhs->GetOwner().lock() == rhs->GetOwner().lock())
                                      {
                                          continue;
                                      }

                                      {
                                          std::lock_guard lock(m_layer_mask_mutex_);
                                          if (!m_layer_mask_[rhs->GetOwner().lock()->GetLayer()].test(
                                               rhs->GetOwner().lock()->GetLayer()))
                                          {
                                              continue;
                                          }
                                      }

                                      rhs_chunk.push_back(rhs->GetSharedPtr<Components::BaseCollider>());
                                  }

                                  CheckCollisionChunk(lhs, rhs_chunk);
                              }
                          }, ap);
    }

    void CollisionDetector::PreUpdate(const float& dt)
    {
        m_collision_map_.merge(m_frame_collision_map_);

        m_frame_collision_map_.clear();
        m_speculation_map_.clear();

        const auto  scene     = GetSceneManager().GetActiveScene().lock();
        const auto  rbs = scene->GetCachedComponents<Components::Rigidbody>();

        for (const auto& rb : rbs)
        {
            const auto lhs = rb.lock();

            if (!lhs)
            {
                continue;
            }

            tbb::parallel_for_each(
                          rbs.range(),
                          [lhs, this](const WeakComponent& p_rhs)
                          {
                              if (lhs == p_rhs.lock())
                              {
                                  return;
                              }

                              const auto rhs = p_rhs.lock();

                              if (!rhs)
                              {
                                  return;
                              }

                              {
                                  std::lock_guard lock(m_layer_mask_mutex_);
                                  if (!m_layer_mask_[lhs->GetOwner().lock()->GetLayer()].test(
                                   rhs->GetOwner().lock()->GetLayer()))
                                  {
                                      return;
                                  }
                              }

                              const auto lhs_cl = lhs->GetSharedPtr<Components::Rigidbody>()->GetMainCollider().lock();
                              const auto rhs_cl = rhs->GetSharedPtr<Components::Rigidbody>()->GetMainCollider().lock();

                              CheckGrounded(lhs_cl, rhs_cl);
                          });
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
                          &out_mutex](const std::pair<const eLayerType, StrongLayer>& layer)
                      {
                          const auto objects = layer.second->GetGameObjects();

                          std::for_each(
                                        std::execution::par, objects.begin(), objects.end(),
                                        [ray, &distance, &out, &out_mutex](const WeakObject& obj)
                                        {
                                            const auto obj_locked = obj.lock();
                                            const auto cls        = obj_locked->GetComponents<Components::BaseCollider>();

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
                              const auto cls = locked->GetComponents<Components::BaseCollider>();

                              std::for_each(
                                            std::execution::par, cls.begin(), cls.end(),
                                            [ray, distance, &hit, &out, &out_mutex, &obj,
                                                locked](const WeakBaseCollider& cl_o)
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
