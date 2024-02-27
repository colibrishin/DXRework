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
          m_layer_mask_[i][j] = true;
          m_layer_mask_[j][i] = true;
        }
        else
        {
          m_layer_mask_[i][j] = false;
          m_layer_mask_[j][i] = false;
        }
      }
    }
  }

  void CollisionDetector::Update(const float& dt) {}

  void CollisionDetector::PreUpdate(const float& dt) {}

  void CollisionDetector::PreRender(const float& dt) {}

  void CollisionDetector::Render(const float& dt)
  {
    OnImGui();
  }

  void CollisionDetector::PostRender(const float& dt) {}

  void CollisionDetector::FixedUpdate(const float& dt)
  {
    if (const auto scene = GetSceneManager().GetActiveScene().lock())
    {
      const auto& tree = scene->GetObjectTree();

      std::stack<const Octree*> stack;
      stack.push(&tree);

      std::vector<std::vector<WeakObject>> node_objects;
      std::map<const Octree*, bool>        visited;

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
        for (int i = 7; i >= 0; --i) { if (children[i]) { stack.push(children[i]); } }

        // If it never visited, then check collision.
        if (!visited[node])
        {
          // Self collision check
          for (int i = 0; i < value.size(); ++i)
          {
            const auto& obj = value[i].lock();
            if (!obj) { continue; }
            const auto& cl = obj->GetComponent<Components::Collider>().lock();

            // If object is inactive or collider is inactive, then dispatch exit event.
            if (obj && !obj->GetActive() || (cl && !cl->GetActive())) { DispatchInactiveExit(value[i]); }

            for (int j = i + 1; j < value.size(); ++j)
            {
              if constexpr (g_speculation_enabled) { TestSpeculation(value[i], value[j], dt); }
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
                if constexpr (g_speculation_enabled) { TestSpeculation(value[j], parent_compare_set[k], dt); }
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
    }

    for (const auto& [lhs, rhs_set] : m_frame_collision_map_)
    {
      m_collision_map_[lhs].insert(rhs_set.begin(), rhs_set.end());

      for (const auto& rhs : rhs_set) { m_collision_map_[rhs].insert(lhs); }
    }

    m_frame_collision_map_.clear();

    // Remove empty set.
    for (auto it = m_collision_map_.begin(); it != m_collision_map_.end();)
    {
      if (it->second.empty()) { it = m_collision_map_.unsafe_erase(it); }
      else { ++it; }
    }
  }

  void CollisionDetector::PostUpdate(const float& dt) {}

  void CollisionDetector::OnImGui()
  {
    if (const auto scene = GetSceneManager().GetActiveScene().lock())
    {
      if (ImGui::Begin("Collision Detector"))
      {
        if (ImGui::TreeNode("Layer Mask"))
        {
          if (ImGui::BeginTable("Layer Mask", LAYER_MAX))
          {
            for (int i = 0; i < LAYER_MAX; ++i)
            {
              ImGui::TableNextRow();
              for (int j = 0; j < LAYER_MAX; ++j)
              {
                ImGui::TableSetColumnIndex(j);
                ImGui::Text("%s\n%s", g_layer_type_str[i], g_layer_type_str[j]);

                const std::string label = std::format("##{}{}", i, j);
                if (ImGui::Checkbox(label.c_str(), &m_layer_mask_[i][j]))
                {
                  if (m_layer_mask_[i][j]) { UnsetCollisionLayer(static_cast<eLayerType>(i), static_cast<eLayerType>(j)); }
                  else { SetCollisionLayer(static_cast<eLayerType>(i), static_cast<eLayerType>(j)); }
                }
              }
            }

            ImGui::EndTable();
          }

          ImGui::TreePop();
        }

        if (ImGui::TreeNode("Collision Info"))
        {
          for (const auto& [lhs, rhs_set] : m_collision_map_)
          {
            for (const auto& rhs : rhs_set)
            {
              if (const auto lhs_obj = scene->FindGameObject(lhs).lock())
              {
                if (const auto rhs_obj = scene->FindGameObject(rhs).lock())
                {
                  ImGui::Text("%s - %s", lhs_obj->GetName().c_str(), rhs_obj->GetName().c_str());
                }
              }
            }
          }

          ImGui::TreePop();
        }

        ImGui::End();
      }
    }
  }

  void CollisionDetector::TestCollision(const WeakObject& p_lhs, const WeakObject& p_rhs)
  {
    const auto lhs = p_lhs.lock();
    const auto rhs = p_rhs.lock();

    if (!lhs || !rhs) { return; }
    if (!IsCollisionLayer(lhs->GetLayer(), rhs->GetLayer())) { return; }
    if (lhs->GetParent().lock() || rhs->GetParent().lock())
    {
      if (lhs->GetParent().lock() == rhs || rhs->GetParent().lock() == lhs) { return; }
      if (lhs->GetParent().lock() == rhs->GetParent().lock()) { return; }
    }
    if (!lhs->GetActive() || !rhs->GetActive()) { return; }

    // Octree sanity check
    if (lhs == rhs) { throw std::logic_error("Self collision detected"); }

    // Speculation caught.
    if (m_frame_collision_map_.contains(lhs->GetID()) &&
        m_frame_collision_map_[lhs->GetID()].contains(rhs->GetID())) { return; }

    const auto ltr = lhs->GetComponent<Components::Transform>().lock();
    const auto rtr = rhs->GetComponent<Components::Transform>().lock();

    const auto lcl = lhs->GetComponent<Components::Collider>().lock();
    const auto rcl = rhs->GetComponent<Components::Collider>().lock();

    if (lcl && rcl)
    {
      if (!lcl->GetActive() || !rcl->GetActive()) { return; }
      const bool collision = Components::Collider::Intersects(lcl, rcl);

      if (collision)
      {
        if (!m_collision_map_.contains(lhs->GetID()) ||
            !m_collision_map_[lhs->GetID()].contains(rhs->GetID()))
        {
          // Initial Collision
          m_frame_collision_map_[lhs->GetID()].insert(rhs->GetID());
          m_frame_collision_map_[rhs->GetID()].insert(lhs->GetID());
        }

        const auto lrb = lhs->GetComponent<Components::Rigidbody>().lock();
        const auto rrb = rhs->GetComponent<Components::Rigidbody>().lock();

        if (lrb && rrb) { m_collision_produce_queue_.push_back({lhs, rhs, false, true}); }

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

  void CollisionDetector::TestSpeculation(const WeakObject& p_lhs, const WeakObject& p_rhs, const float dt)
  {
    auto lhs = p_lhs.lock();
    auto rhs = p_rhs.lock();

    if (!lhs || !rhs) { return; }
    if (!IsCollisionLayer(lhs->GetLayer(), rhs->GetLayer())) { return; }
    if (lhs->GetParent().lock() || rhs->GetParent().lock())
    {
      if (lhs->GetParent().lock() == rhs || rhs->GetParent().lock() == lhs) { return; }
      if (lhs->GetParent().lock() == rhs->GetParent().lock()) { return; }
    }
    if (!lhs->GetActive() || !rhs->GetActive()) { return; }

    // Octree sanity check
    if (lhs == rhs) { throw std::logic_error("Self collision detected"); }
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
    if (!lrb && !rrb) { return; }
    // Move rigid-body object to lhs or fixed object to rhs.
    if ((rrb && !lrb) || (lrb && lrb->IsFixed()))
    {
      std::swap(lhs, rhs);
      std::swap(ltr, rtr);
      std::swap(lcl, rcl);
      std::swap(lrb, rrb);
    }

    // If rhs was not exist, then skip.
    if (!lrb) { return; }

    if (lcl && rcl)
    {
      // If any of object collider is disabled, then skip.
      if (!lcl->GetActive() || !rcl->GetActive()) { return; }

      bool collision1 = false;
      bool collision2 = false;

      // lhs test
      const auto lvel = Engine::Physics::EvalT1PositionDelta(lrb->GetT0LinearVelocity(), lrb->GetT0Force(), dt);
      Vector3    ldir;
      lvel.Normalize(ldir);

      // no need to check if velocity is zero.
      if (ldir != Vector3::Zero || !FloatCompare
          (lvel.Length(), 0.f) || !lrb->GetActive())
      {
        collision1 = Components::Collider::Intersects
          (lcl, rcl, lvel.Length(), ldir);
      }

      // rhs test, if exists.
      if (rrb && rrb->GetActive())
      {
        const auto rvel = Engine::Physics::EvalT1PositionDelta(rrb->GetT0LinearVelocity(), rrb->GetT0Force(), dt);
        Vector3    rdir;
        rvel.Normalize(rdir);
        if (rdir != Vector3::Zero || !FloatCompare
            (rvel.Length(), 0.f)) { collision2 = Components::Collider::Intersects(rcl, lcl, rvel.Length(), rdir); }
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

          m_collision_produce_queue_.push_back({lhs, rhs, true, true});
        }

        // Or continuous collision
        lhs->DispatchComponentEvent(rcl);
        rhs->DispatchComponentEvent(lcl);
        lcl->AddCollidedObject(rhs->GetID());
        rcl->AddCollidedObject(lhs->GetID());
      }
    }
  }

  void CollisionDetector::DispatchInactiveExit(const WeakObject& lhs)
  {
    const auto lcl   = lhs.lock()->GetComponent<Components::Collider>().lock();
    const auto scene = GetSceneManager().GetActiveScene().lock();

    if (!lcl) { return; }

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

            lhs.lock()->DispatchComponentEvent(rcl);
            rhs->DispatchComponentEvent(lcl);
          }
        }
      }
    }
  }

  void CollisionDetector::SetCollisionLayer(
    const eLayerType a,
    const eLayerType b
  )
  {
    m_layer_mask_[a][b] = true;
    m_layer_mask_[b][a] = true;
  }

  void CollisionDetector::UnsetCollisionLayer(eLayerType layer, eLayerType layer2)
  {
    m_layer_mask_[layer][layer2] = false;
    m_layer_mask_[layer2][layer] = false;
  }

  bool CollisionDetector::IsCollisionLayer(eLayerType layer1, eLayerType layer2)
  {
    std::lock_guard l(m_layer_mask_mutex_);
    return m_layer_mask_[layer1][layer2];
  }

  bool CollisionDetector::IsCollided(GlobalEntityID id) const
  {
    if (!m_collision_map_.contains(id)) { return false; }

    return !m_collision_map_.at(id).empty();
  }

  bool CollisionDetector::IsCollided(GlobalEntityID id1, GlobalEntityID id2) const
  {
    if (!m_collision_map_.contains(id1)) { return false; }

    return m_collision_map_.at(id1).contains(id2);
  }

  concurrent_vector<CollisionInfo>& CollisionDetector::GetCollisionInfo() { return m_collision_produce_queue_; }

  bool CollisionDetector::IsCollidedInFrame(GlobalEntityID id1, GlobalEntityID id2) const
  {
    if (!m_frame_collision_map_.contains(id1)) { return false; }

    return m_frame_collision_map_.at(id1).contains(id2);
  }
} // namespace Engine::Manager
