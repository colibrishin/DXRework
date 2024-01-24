#include "pch.h"
#include "clPlayer.h"

#include "clCharacterController.hpp"
#include "clDarkScene.h"
#include "clHitbox.h"
#include "clRifile.h"
#include "egAnimator.h"
#include "egBaseCollider.hpp"
#include "egBoneAnimation.h"
#include "egMaterial.h"
#include "egModelRenderer.h"
#include "egRigidbody.h"
#include "egSceneManager.hpp"
#include "egShader.hpp"
#include "egShape.h"
#include "egTransform.h"

SERIALIZER_ACCESS_IMPL
(
 Client::Object::Player,
 _ARTAG(_BSTSUPER(Object))
)

namespace Client::Object
{
  void Player::Initialize()
  {
    Object::Initialize();

    const auto model = Resources::Shape::Get("CharacterShape").lock();
    SetName("Player");

    const auto mr = AddComponent<Components::ModelRenderer>().lock();
    mr->SetMaterial(Resources::Material::Get("Character"));

    const auto tr   = AddComponent<Components::Transform>().lock();
    const auto cldr = AddComponent<Components::Collider>().lock();
    const auto rb   = AddComponent<Components::Rigidbody>().lock();
    const auto atr  = AddComponent<Components::Animator>().lock();

    cldr->SetModel(model);
    cldr->SetType(BOUNDING_TYPE_BOX);
    cldr->SetMass(1.0f);

    rb->SetFrictionCoefficient(0.1f);
    rb->SetGravityOverride(true);

    atr->SetAnimation(0);

    const auto rifle = GetScene().lock()->CreateGameObject<Rifle>(LAYER_DEFAULT);
    AddChild(rifle);

    const auto bb_map = model->GetBoneBoundingBoxes();

    for (const auto& [idx, box] : bb_map)
    {
      const auto child = GetScene().lock()->CreateGameObject<Objects::Hitbox>(LAYER_HITBOX).lock();
      child->SetBoundingBox(box);
      child->SetName("Bone" + std::to_string(idx));
      AddChild(child);
      m_child_bones_[idx] = child->GetLocalID();

      if (child->GetName() == "Bone5") { m_head_ = child; }
    }

    AddComponent<State::CharacterController>();
  }

  void Player::PreUpdate(const float& dt) { Object::PreUpdate(dt); }

  void Player::Update(const float& dt)
  {
    Object::Update(dt);

    if (GetApplication().GetKeyState().IsKeyDown(Keyboard::Space))
    {
      GetSceneManager().SetActive<Scene::DarkScene>("Thunder");
    }
  }

  void Player::PreRender(const float& dt) { Object::PreRender(dt); }

  void Player::Render(const float& dt) { Object::Render(dt); }

  void Player::PostRender(const float& dt) { Object::PostRender(dt); }

  void Player::UpdateHitboxes()
  {
    const auto cl  = GetComponent<Components::Collider>().lock();
    const auto mr  = GetComponent<Components::ModelRenderer>().lock();
    const auto atr = GetComponent<Components::Animator>().lock();

    if (!cl || !mr || !atr) { return; }

    const auto mtl = mr->GetMaterial().lock();

    if (!mtl) { return; }

    const auto anim   = mtl->GetResource<Resources::BoneAnimation>(atr->GetAnimation()).lock();
    auto       deform = anim->GetFrameAnimation(atr->GetFrame());
    const auto rb     = GetComponent<Components::Rigidbody>().lock();
    Vector3    min    = {FLT_MAX, FLT_MAX, FLT_MAX};
    Vector3    max    = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (const auto& [idx, id] : m_child_bones_)
    {
      const auto child = GetChild(id).lock();

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

  void Player::FixedUpdate(const float& dt)
  {
    Object::FixedUpdate(dt);
    UpdateHitboxes();
  }

  WeakObject Player::GetHead() const { return m_head_; }
}
