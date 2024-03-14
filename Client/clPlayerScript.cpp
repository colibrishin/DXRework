#include "pch.h"
#include "clPlayerScript.h"

#include "clButtonScript.h"
#include "clHitboxScript.hpp"
#include "egAnimator.h"
#include "egBaseCollider.hpp"
#include "egBoneAnimation.h"
#include "egCamera.h"
#include "egCollisionDetector.h"
#include "egImGuiHeler.hpp"
#include "egMaterial.h"
#include "egModelRenderer.h"
#include "egMouseManager.h"
#include "egRigidbody.h"
#include "egShape.h"
#include "egText.h"

SERIALIZER_ACCESS_IMPL
(
 Client::Scripts::PlayerScript,
 _ARTAG(_BSTSUPER(Script)) _ARTAG(m_hp_) _ARTAG(m_state_) _ARTAG(m_prev_state_)
 _ARTAG(m_top_view_) _ARTAG(m_cam_id_) _ARTAG(m_shoot_interval)
 _ARTAG(m_bone_initialized_) _ARTAG(m_rifle_initialized_) _ARTAG(m_health_initialized_)
 _ARTAG(m_child_bones_)
)

namespace Client::Scripts
{
  void PlayerScript::Initialize()
  {
    Script::Initialize();
    SetState(CHAR_STATE_IDLE);

    const auto obj = GetOwner().lock();
    const auto model = Resources::Shape::Get("CharacterShape").lock();

    const auto mr = GetOwner().lock()->AddComponent<Components::ModelRenderer>().lock();
    mr->SetMaterial(Resources::Material::Get("Character"));

    const auto tr = obj->AddComponent<Components::Transform>().lock();
    const auto cldr = obj->AddComponent<Components::Collider>().lock();
    const auto rb = obj->AddComponent<Components::Rigidbody>().lock();
    const auto atr = obj->AddComponent<Components::Animator>().lock();

    cldr->SetShape(model);
    cldr->SetType(BOUNDING_TYPE_BOX);
    cldr->SetMass(1.0f);

    rb->SetFrictionCoefficient(0.1f);
    rb->SetGravityOverride(true);
    rb->SetNoAngular(true);

    atr->SetAnimation(0);

    // Rifle initialization
    // todo: swappable weapons
    if (!m_rifle_initialized_)
    {
      const auto rifle = obj->GetScene().lock()->CreateGameObject<Abstract::Object>(GetOwner().lock()->GetLayer()).lock();
      GetOwner().lock()->AddChild(rifle);

      const auto rifle_model = Resources::Shape::Get("RifleShape").lock();

      const auto cmr = rifle->AddComponent<Components::ModelRenderer>().lock();

      cmr->SetMaterial(Resources::Material::Get("ColorRifle"));

      const auto ctr   = rifle->AddComponent<Components::Transform>().lock();
      const auto catr  = rifle->AddComponent<Components::Animator>().lock();
      const auto ccldr = rifle->AddComponent<Components::Collider>().lock();

      ctr->SetSizeAbsolute(true);
      ctr->SetRotateAbsolute(false);
      catr->SetAnimation(0);
      ccldr->SetShape(rifle_model);
      m_rifle_initialized_ = true;
    }

    // Bone Initialization
    if (!m_bone_initialized_) 
    {
      const auto bb_map = model->GetBoneBoundingBoxes();

      for (const auto& [idx, box] : bb_map)
      {
        const auto child = obj->GetScene().lock()->CreateGameObject<Abstract::Object>(LAYER_HITBOX).lock();
        const auto ctr = child->AddComponent<Components::Transform>().lock();
        child->AddComponent<Components::Collider>();
        child->AddScript<HitboxScript>();

        ctr->SetLocalPosition(box.Center);
        ctr->SetLocalScale(Vector3(box.Extents) * 2.f);
        ctr->SetLocalRotation(box.Orientation);

        child->SetName("Bone" + std::to_string(idx));
        obj->AddChild(child);
        m_child_bones_[idx] = child->GetLocalID();

        if (child->GetName() == "Bone5")
        {
          m_head_ = child;
        }
      }

      m_bone_initialized_ = true;
    }

    // Health Text
    if (!m_health_initialized_) 
    {
      const auto child = obj->GetScene().lock()->CreateGameObject<Objects::Text>(LAYER_UI, Resources::Font::Get("DefaultFont")).lock();
      child->SetText("");
      child->SetPosition({0.f, 64.f});
      child->SetColor({1.f, 1.f, 1.f, 1.f});
      child->SetScale(1.f);
      m_health_initialized_ = true;
    }

    // todo: determine local player
    //MoveCameraToChild(true);
  }

  void PlayerScript::PreUpdate(const float& dt)
  {
    m_prev_state_ = m_state_;
    CheckGround();
  }

  void PlayerScript::Update(const float& dt)
  {
    const auto rb = GetOwner().lock()->GetComponent<Components::Rigidbody>().lock();

    if (!rb) { return; }

    if constexpr (Engine::g_debug_observer)
    {
      if (const auto head = m_head_.lock() && !m_top_view_)
      {
        const auto head_tr = m_head_.lock()->GetComponent<Components::Transform>().lock();
        const auto mouse_y = GetMouseManager().GetMouseYRotation();
        head_tr->SetLocalRotation(mouse_y);
      }

      const auto body_tr = GetOwner().lock()->GetComponent<Components::Transform>().lock();
      const auto mouse_x = GetMouseManager().GetMouseXRotation();

      body_tr->SetLocalRotation(mouse_x);
    }

    CheckJump(rb);
    CheckMove(rb);
    CheckAttack(dt);
    CheckInteraction();

    switch (GetState())
    {
    case CHAR_STATE_IDLE: if (HasStateChanged()) { GetDebugger().Log("Idle"); }
      break;
    case CHAR_STATE_WALK: if (HasStateChanged()) { GetDebugger().Log("Walk"); }
      break;
    case CHAR_STATE_RUN: break;
    case CHAR_STATE_JUMP: if (HasStateChanged()) { GetDebugger().Log("Jump"); }
      break;
    case CHAR_STATE_ATTACK: if (HasStateChanged()) { GetDebugger().Log("Attack"); }
      break;
    case CHAR_STATE_DIE: break;
    case CHAR_STATE_HIT: break;
    case CHAR_STATE_MAX:
    default: break;
    }
  }

  void PlayerScript::PostUpdate(const float& dt) {}

  void PlayerScript::FixedUpdate(const float& dt)
  {
    UpdateHitbox();
  }

  void PlayerScript::PreRender(const float& dt) {}

  void PlayerScript::Render(const float& dt) {}

  void PlayerScript::PostRender(const float& dt) {}

  void PlayerScript::SetActive(bool active)
  {
    MoveCameraToChild(active);
    Script::SetActive(active);
  }

  void PlayerScript::OnDeserialized()
  {
    Script::OnDeserialized();
    if (m_bone_initialized_)
    {
      const auto& children = GetOwner().lock()->GetChildren();

      for (const auto& child : children)
      {
        if (const auto candidate = child.lock())
        {
          if (candidate->GetName() == "Bone5")
          {
            m_head_ = candidate;
          }
        }
      }
    }
  }

  void PlayerScript::OnImGui()
  {
    Script::OnImGui();
    Engine::intDisabled("Current State", m_state_);
    Engine::intDisabled("Previous State", m_prev_state_);
    Engine::CheckboxAligned("Top View", m_top_view_);
    Engine::lldDisabled("Camera ID", m_cam_id_);
    Engine::FloatAligned("Shoot Interval", m_shoot_interval);
    Engine::FloatAligned("HP", m_hp_);
  }

  UINT PlayerScript::GetHealth() const { return m_hp_; }

  void PlayerScript::Hit(const float damage)
  {
    m_hp_ -= damage;

    if (m_hp_ <= 0.f)
    {
      SetState(CHAR_STATE_DIE);
    }
    else
    {
      SetState(CHAR_STATE_HIT);
    }
  }

  PlayerScript::PlayerScript() : m_state_(CHAR_STATE_IDLE), m_prev_state_(CHAR_STATE_IDLE), m_bone_initialized_(false),
                                 m_rifle_initialized_(false), m_health_initialized_(false), m_top_view_(false),
                                 m_cam_id_(0), m_shoot_interval(0.3f), m_hp_(100.f) {}

  void PlayerScript::Hitscan(const float damage, const float range) const
  {
    const auto head_tr = m_head_.lock()->GetComponent<Components::Transform>().lock();
    const auto owner = GetOwner().lock();
    const auto lcl = GetOwner().lock()->GetComponent<Components::Collider>().lock();
    const auto start   = head_tr->GetWorldPosition();
    const auto forward = head_tr->Forward();

    const auto end = start + forward * range;

    if (const auto scene = GetOwner().lock()->GetScene().lock())
    {
      const auto& tree = scene->GetObjectTree();

      std::queue<const Octree*> queue;
      queue.push(&tree);

      while (!queue.empty())
      {
        const auto  node     = queue.front();
        const auto& value    = node->Read();
        const auto& children = node->Next();
        const auto& active   = node->ActiveChildren();
        queue.pop();

        for (const auto& p_rhs : value)
        {
          if (const auto& rhs = p_rhs.lock())
          {
            if (rhs->GetParent().lock() == owner) { continue; }
            if (rhs == owner) { continue; }

            if (const auto& rcl = rhs->GetComponent<Components::Collider>().lock())
            {
              if (rcl->GetActive())
              {
                float dist = 0.f;

                if (rcl->Intersects(start, forward, range, dist))
                {
                  if (const auto script = rhs->GetScript<Scripts::HitboxScript>().lock())
                  {
                    GetDebugger().Log(
                        std::format(
                            "Hit {} for {} damage", 
                            rhs->GetName(), 
                            damage));

                    script->Hit(damage);
                  }
                  else
                  {
                    rhs->DispatchComponentEvent(lcl);
                  }
                }
              }
            }
          }
        }

        // Add children to stack.
        for (int i = 7; i >= 0; --i) { if (children[i] && children[i]->Contains(end)) { queue.push(children[i]); } }
      }
    }
  }

  eCharacterState PlayerScript::GetState() const
  {
    return m_state_;
  }

  void PlayerScript::SetState(const eCharacterState state)
  {
    m_prev_state_ = m_state_;
    m_state_ = state;
  }

  bool PlayerScript::HasStateChanged() const { return m_state_ != m_prev_state_; }

  void PlayerScript::MoveCameraToChild(bool active)
  {
    if (const auto head = m_head_.lock(); 
        const auto scene = GetOwner().lock()->GetScene().lock())
    {
      const auto cam = scene->GetMainCamera().lock();

      if (head && scene && cam && !active)
      {
        head->DetachChild(m_cam_id_);
        m_cam_id_ = g_invalid_id;
      }
      else if (head && scene && cam && active)
      {
        head->AddChild(cam);
        m_cam_id_ = cam->GetLocalID();
      }
    }
  }

  void PlayerScript::SetHeadView(const bool head_view)
  {
    if (m_cam_id_ == g_invalid_id) { return; }

    const auto head = m_head_.lock();

    if (!head) { return; }

    const auto cam_obj = head->GetChild(m_cam_id_).lock();

    if (!cam_obj) { return; }

    const auto cam = cam_obj->GetSharedPtr<Objects::Camera>();
    const auto cam_tr = cam_obj->GetComponent<Components::Transform>().lock();

    if (m_top_view_)
    {
      cam->SetOrthogonal(false);
      cam_tr->SetLocalPosition({0.f, 10.f, 0.f});
      cam_tr->SetLocalRotation(Quaternion::CreateFromAxisAngle(Vector3::Right, XM_PIDIV2));
    }
    else
    {
      cam->SetOrthogonal(false);
      cam_tr->SetLocalPosition(Vector3::Zero);
      cam_tr->SetLocalRotation(Quaternion::Identity);
    }

    m_top_view_ = head_view;
  }

  void PlayerScript::CheckJump(const boost::shared_ptr<Components::Rigidbody>& rb)
  {
    if (!rb->GetGrounded()) { SetState(CHAR_STATE_JUMP); }
    else { SetState(CHAR_STATE_IDLE); }
  }

  void PlayerScript::CheckMove(const boost::shared_ptr<Components::Rigidbody>& rb)
  {
    if constexpr (Engine::g_debug_observer)
    {
      return;
    }

    float      speed = 10.0f;
    const auto scene = GetOwner().lock()->GetScene().lock();

    auto forward = GetOwner().lock()->GetComponent<Components::Transform>().lock()->Forward();
    Vector3 ortho;
    forward.Cross(Vector3::Up, ortho);

    forward *= {1.f, 0.f, 1.f};
    ortho *= {1.f, 0.f, 1.f};

    forward *= speed;
    ortho *= speed;

    bool pressed = false;
    const auto atr = GetOwner().lock()->GetComponent<Components::Animator>().lock();

    constexpr UINT forward_anim = 20;
    constexpr UINT backward_anim = 19;
    constexpr UINT left_anim = 22;
    constexpr UINT right_anim = 21;
    constexpr UINT idle_anim = 0;

    if (GetApplication().GetKeyState().IsKeyDown(Keyboard::W))
    {
      atr->SetAnimation(forward_anim);
      rb->AddT1Force(forward);
      pressed = true;
    }

    if (GetApplication().GetKeyState().IsKeyDown(Keyboard::A))
    {
      atr->SetAnimation(left_anim);
      rb->AddT1Force(ortho);
      pressed = true;
    }

    if (GetApplication().GetKeyState().IsKeyDown(Keyboard::S))
    {
      atr->SetAnimation(backward_anim);
      rb->AddT1Force(-forward);
      pressed = true;
    }

    if (GetApplication().GetKeyState().IsKeyDown(Keyboard::D))
    {
      atr->SetAnimation(right_anim);
      rb->AddT1Force(-ortho);
      pressed = true;
    }

    if (!pressed)
    {
      SetState(CHAR_STATE_IDLE);
      atr->SetAnimation(idle_anim);
    }
    else { SetState(CHAR_STATE_WALK); }
  }

  void PlayerScript::CheckAttack(const float& dt)
  {
    if constexpr (Engine::g_debug_observer)
    {
      return;
    }

    if (GetApplication().GetMouseState().leftButton)
    {
      SetState(CHAR_STATE_ATTACK);

      if (m_shoot_interval < 0.5f)
      {
        m_shoot_interval += dt;
        return;
      }

      m_shoot_interval = 0.f;
      const auto tr    =
        GetOwner().lock()->GetComponent<Components::Transform>().lock();

      Ray ray;
      ray.position  = tr->GetWorldPosition();
      ray.direction = tr->Forward();

      constexpr float distance = 5.f;

      GetDebugger().Draw(ray, Colors::AliceBlue);
      std::vector<WeakObject> out;

      Hitscan(10.f, 10.f);
    }
  }

  void PlayerScript::CheckGround() const
  {
    const auto  scene = GetOwner().lock()->GetScene().lock();
    const auto& tree  = scene->GetObjectTree();
    const auto  rb    = GetOwner().lock()->GetComponent<Components::Rigidbody>().lock();

    std::queue<const Octree*> q;
    q.push(&tree);

    while (!q.empty())
    {
      const auto node = q.front();
      q.pop();

      const auto& value    = node->Read();
      const auto& children = node->Next();

      for (const auto v : value)
      {
        const auto lcl = GetOwner().lock()->GetComponent<Components::Collider>().lock();
        const auto rcl = v.lock()->GetComponent<Components::Collider>().lock();

        if (!GetCollisionDetector().IsCollisionLayer(GetOwner().lock()->GetLayer(), v.lock()->GetLayer())) { continue; }
        if (!rcl || lcl == rcl) { continue; }

        const auto owner = rcl->GetOwner().lock();
        const auto owner_parent = owner->GetParent();

        if (owner_parent.lock() == GetOwner().lock()) { continue; }

        if (Components::Collider::Intersects(lcl, rcl, Vector3::Down))
        {
          rb->SetGrounded(true);
          return;
        }
      }

      for (const auto& child : children)
      {
        if (child &&
            child->Contains
            (GetOwner().lock()->GetComponent<Components::Transform>().lock()->GetWorldPosition())) { q.push(child); }
      }
    }
  }

  void PlayerScript::CheckInteraction() const
  {
    if (GetApplication().GetKeyState().E)
    {
      const auto scene = GetOwner().lock()->GetScene().lock();
      const auto& tree = scene->GetObjectTree();

      const auto& watch_position = m_head_.lock()->GetComponent<Components::Transform>().lock()->GetWorldPosition();
      const auto& watch_forward = m_head_.lock()->GetComponent<Components::Transform>().lock()->Forward();

      bool pressed = false;
      std::queue<const Octree*> q;
      q.push(&tree);

      while (!q.empty())
      {
        const auto node = q.front();
        q.pop();

        const auto& value    = node->Read();
        const auto& children = node->Next();

        for (const auto v : value)
        {
          const auto lcl = m_head_.lock()->GetComponent<Components::Collider>().lock();
          const auto rcl = v.lock()->GetComponent<Components::Collider>().lock();

          if (!GetCollisionDetector().IsCollisionLayer
            (GetOwner().lock()->GetLayer(), v.lock()->GetLayer())) { continue; }
          if (!rcl || lcl == rcl) { continue; }

          const auto owner        = rcl->GetOwner().lock();
          const auto owner_parent = owner->GetParent();

          if (owner_parent.lock() == GetOwner().lock()) { continue; }

          const auto& lbnd = rcl->GetBounding();
          const auto& rbnd = lcl->GetBounding();

          float dist = 0;

          if (rbnd.TestRay(rbnd, watch_forward, dist))
          {
            if (const auto script = owner->GetScript<Scripts::ButtonScript>().lock())
            {
              script->Press();
              pressed = true;
              break;
            }
          }
        }

        if (pressed) { break; }

        for (const auto& child : children)
        {
          if (child &&
              child->Contains
              (GetOwner().lock()->GetComponent<Components::Transform>().lock()->GetWorldPosition())) { q.push(child); }
        }
      }
    }
  }

  void PlayerScript::UpdateHitbox()
  {
    const auto obj = GetOwner().lock();
    const auto cl  = obj->GetComponent<Components::Collider>().lock();
    const auto mr  = obj->GetComponent<Components::ModelRenderer>().lock();
    const auto atr = obj->GetComponent<Components::Animator>().lock();

    if (!cl || !mr || !atr) { return; }

    const auto mtl = mr->GetMaterial().lock();

    if (!mtl) { return; }

    if (const auto anim = mtl->GetResource<Resources::BoneAnimation>(atr->GetAnimation()).lock())
    {
      auto       deform = anim->GetFrameAnimation(atr->GetFrame());
      const auto rb     = obj->GetComponent<Components::Rigidbody>().lock();
      Vector3    min    = {FLT_MAX, FLT_MAX, FLT_MAX};
      Vector3    max    = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

      for (const auto& [idx, id] : m_child_bones_)
      {
        const auto child = obj->GetChild(id).lock();

        const auto ctr = child->GetComponent<Components::Transform>().lock();
        ctr->SetAnimationMatrix(deform[idx]);

        static const std::vector<Vector3> stock_vertices
        {
          {0.5f, 0.5f, 0.5f},
          {0.5f, -0.5f, 0.5f},
          {-0.5f, -0.5f, 0.5f},
          {-0.5f, 0.5f, 0.5f},
          {0.5f, 0.5f, -0.5f},
          {0.5f, -0.5f, -0.5f},
          {-0.5f, -0.5f, -0.5f},
          {-0.5f, 0.5f, -0.5f},
        };

        std::vector<Vector3> out_vertices;
        out_vertices.resize(stock_vertices.size());

        XMVector3TransformCoordStream
          (
           out_vertices.data(),
           sizeof(Vector3),
           stock_vertices.data(),
           sizeof(Vector3),
           stock_vertices.size(),
           ctr->GetLocalMatrix()
          );

        for (const auto& v : out_vertices)
        {
          min = Vector3::Min(min, v);
          max = Vector3::Max(max, v);
        }
      }

      BoundingOrientedBox new_obb;
      BoundingBox         bb;
      BoundingBox::CreateFromPoints(bb, min, max);
      BoundingOrientedBox::CreateFromBoundingBox(new_obb, bb);
      cl->SetBoundingBox(new_obb);
    }
  }
}
