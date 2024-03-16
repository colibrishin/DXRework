#include "pch.h"
#include "clPlayerHitboxScript.h"

#include <egBaseCollider.hpp>
#include <egObject.hpp>
#include <egShape.h>
#include <egTransform.h>

#include <egAnimator.h>
#include <egMaterial.h>
#include <egModelRenderer.h>

#include <egBoneAnimation.h>
#include <egRigidbody.h>

SERIALIZE_IMPL
(
  Client::Scripts::PlayerHitboxScript,
  _ARTAG(_BSTSUPER(Engine::Script))
)

namespace Client::Scripts
{
  SCRIPT_CLONE_IMPL(PlayerHitboxScript)

  PlayerHitboxScript::PlayerHitboxScript(const WeakObjectBase& owner)
    : Script(SCRIPT_T_PLAYER_HITBOX, owner) {}

  void PlayerHitboxScript::Initialize()
  {
    const auto& obj = GetOwner().lock();
    const auto model = Resources::Shape::Get("CharacterShape").lock();
    const auto& bb_map = model->GetBoneBoundingBoxes();

    for (const auto& [idx, box] : bb_map)
    {
      const auto child = obj->GetScene().lock()->CreateGameObject<Object>(LAYER_HITBOX).lock();
      const auto ctr = child->AddComponent<Components::Transform>().lock();
      child->AddComponent<Components::Collider>();

      ctr->SetLocalPosition(box.Center);
      ctr->SetLocalScale(Vector3(box.Extents) * 2.f);
      ctr->SetLocalRotation(box.Orientation);

      child->SetName("Bone" + std::to_string(idx));
      obj->AddChild(child);

      if (child->GetName() == "Bone5")
      {
        m_head_hitbox_ = child;
      }
    }
  }

  void PlayerHitboxScript::PreUpdate(const float& dt) {}

  void PlayerHitboxScript::Update(const float& dt) {}

  void PlayerHitboxScript::FixedUpdate(const float& dt)
  {
    updateHitbox();
  }

  void PlayerHitboxScript::PreRender(const float& dt) {}

  void PlayerHitboxScript::Render(const float& dt) {}

  void PlayerHitboxScript::PostRender(const float& dt) {}

  void PlayerHitboxScript::PostUpdate(const float& dt) {}

  void PlayerHitboxScript::OnSerialized()
  {
    Script::OnSerialized();
  }

  void PlayerHitboxScript::OnDeserialized()
  {
    Script::OnDeserialized();

    GetTaskScheduler().AddTask
      (
       TASK_SCRIPT_EVENT, {}, [this](const auto& params, const auto dt)
       {
         if (const auto& owner = GetOwner().lock())
         {
           for (const auto& child : owner->GetChildren())
           {
             if (const auto& locked = child.lock())
             {
               if (locked->GetName().find("Bone5") != std::string::npos) { m_head_hitbox_ = locked; }
             }
           }
         }
       }
      );
  }

  void PlayerHitboxScript::OnImGui() {}

  WeakObjectBase PlayerHitboxScript::GetHead() const
  {
    return m_head_hitbox_;
  }

  PlayerHitboxScript::PlayerHitboxScript()
    : Script(SCRIPT_T_PLAYER_HITBOX, {}) {}

  void PlayerHitboxScript::updateHitbox() const
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
      const auto       deform = anim->GetFrameAnimation(atr->GetFrame());
      const auto rb     = obj->GetComponent<Components::Rigidbody>().lock();
      Vector3    min    = {FLT_MAX, FLT_MAX, FLT_MAX};
      Vector3    max    = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

      for (const auto& child : GetOwner().lock()->GetChildren())
      {
        if (const auto locked = child.lock(); 
            locked && locked->GetName().find("Bone") != std::string::npos)
        {
          const auto ctr = locked->GetComponent<Components::Transform>().lock();
          const auto idx = std::stoi(locked->GetName().substr(4));

          ctr->SetAnimationMatrix(deform[idx]);

          static constexpr Vector3 stock_vertices[]
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
          out_vertices.resize(std::size(stock_vertices));

          XMVector3TransformCoordStream
            (
             out_vertices.data(),
             sizeof(Vector3),
             stock_vertices,
             sizeof(Vector3),
             std::size(stock_vertices),
             ctr->GetLocalMatrix()
            );

          for (const auto& v : out_vertices)
          {
            min = Vector3::Min(min, v);
            max = Vector3::Max(max, v);
          }
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
