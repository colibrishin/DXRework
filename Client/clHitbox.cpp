#include "pch.h"
#include "clHitbox.h"

#include "clCharacterController.hpp"
#include "clHitboxScript.hpp"
#include "egTransform.h"
#include "egBaseCollider.hpp"

SERIALIZER_ACCESS_IMPL
(
 Client::Objects::Hitbox,
 _ARTAG(_BSTSUPER(Engine::Abstract::Object))
 _ARTAG(m_bounding_box_)
)

namespace Client::Objects
{
  Vector3& operator*(XMFLOAT3& lhs, float rhs)
  {
    auto& cast_vec = *reinterpret_cast<Vector3*>(&lhs);
    cast_vec *= rhs;
    return cast_vec;
  }

  void Hitbox::Initialize()
  {
    Object::Initialize();
    AddComponent<Components::Transform>();
    AddComponent<Components::Collider>();
    AddScript<Scripts::HitboxScript>();
  }

  void Hitbox::SetBoundingBox(const BoundingOrientedBox& box)
  {
    m_bounding_box_ = box;
    const auto tr = GetComponent<Components::Transform>().lock();
    tr->SetLocalPosition(m_bounding_box_.Center);
    tr->SetLocalRotation(m_bounding_box_.Orientation);
    tr->SetLocalScale(m_bounding_box_.Extents * 2.f);
  }

  void Hitbox::OnCollisionContinue(const StrongCollider& other)
  {
    Object::OnCollisionContinue(other);
  }

  void Hitbox::OnCollisionEnter(const StrongCollider& other)
  {
    Object::OnCollisionEnter(other);
  }

  void Hitbox::OnCollisionExit(const StrongCollider& other)
  {
    Object::OnCollisionExit(other);
  }
}
