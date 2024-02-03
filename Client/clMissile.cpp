#include "pch.h"
#include "clMissile.h"

#include "egBaseCollider.hpp"
#include "egMaterial.h"
#include "egModelRenderer.h"
#include "egRigidbody.h"
#include "egShape.h"
#include "egTransform.h"

SERIALIZER_ACCESS_IMPL
(
 Client::Object::Missile,
 _ARTAG(_BSTSUPER(Object))
)

namespace Client::Object
{
  void Missile::Initialize()
  {
    Object::Initialize();

    AddComponent<Components::Transform>();
    AddComponent<Components::Collider>();
    const auto rb = AddComponent<Components::Rigidbody>().lock();
    rb->SetGravityOverride(false);

    const auto mtr = AddComponent<Components::ModelRenderer>().lock();
    mtr->SetMaterial(Resources::Material::Get("ColorMissile"));
  }

  void Missile::PostRender(const float& dt) { Object::PostRender(dt); }

  void Missile::PostUpdate(const float& dt) { Object::PostUpdate(dt); }

  void Missile::PreRender(const float& dt) { Object::PreRender(dt); }

  void Missile::PreUpdate(const float& dt) { Object::PreUpdate(dt); }

  void Missile::Render(const float& dt) { Object::Render(dt); }

  void Missile::Update(const float& dt)
  {
    Object::Update(dt);

    const auto rb = GetComponent<Components::Rigidbody>().lock();

    auto dir = rb->GetLinearMomentum();
    dir.Normalize();
    auto major = dir.Cross(Vector3::Down);
    major.Normalize();
    auto minor = major.Cross(dir);
    minor.Normalize();

    const Matrix rot =
    {
      major.x, major.y, major.z, 0.f,
      dir.x, dir.y, dir.z, 0.f,      
      minor.x, minor.y, minor.z, 0.f,
      0.f, 0.f, 0.f, 1.f
    };

    const auto tr = GetComponent<Components::Transform>().lock();
    tr->SetLocalRotation(Quaternion::CreateFromRotationMatrix(rot));
  }
}
