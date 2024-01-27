#include "pch.h"
#include "clTestCube.hpp"

#include <egCubeMesh.h>

#include "egBaseCollider.hpp"
#include "egComputeShader.h"
#include "egMaterial.h"
#include "egModelRenderer.h"
#include "egParticleRenderer.h"
#include "egRigidbody.h"
#include "egShape.h"
#include "egSound.h"
#include "egSoundPlayer.h"
#include "egTransform.h"

SERIALIZER_ACCESS_IMPL
(
 Client::Object::TestCube,
 _ARTAG(_BSTSUPER(Engine::Abstract::Object))
)

namespace Client::Object
{
  inline TestCube::TestCube()
    : Object() {}

  inline void TestCube::Initialize()
  {
    SetName("TestCube");
    const auto mr = AddComponent<Components::ModelRenderer>().lock();
    mr->SetMaterial(Resources::Material::Get("NormalLightCube"));

    AddComponent<Components::Transform>();
    AddComponent<Components::Collider>();
    const auto cldr = GetComponent<Components::Collider>().lock();
    cldr->SetType(BOUNDING_TYPE_BOX);
    cldr->SetMass(1.0f);

    AddComponent<Components::Rigidbody>();
    const auto rb = GetComponent<Components::Rigidbody>().lock();

    rb->SetFrictionCoefficient(0.1f);
    rb->SetGravityOverride(true);

    const auto snd = AddComponent<Components::SoundPlayer>().lock();
    snd->SetSound(GetResourceManager().GetResource<Resources::Sound>("AmbientSound").lock());
    snd->PlaySound();

    {
      const auto child = GetScene().lock()->CreateGameObject<Object>(GetLayer()).lock();
      child->SetName("CubeParticleChild");
      child->AddComponent<Components::Transform>();

      const auto c_pr = child->AddComponent<Components::ParticleRenderer>().lock();
      c_pr->SetComputeShader(Resources::ComputeShader::Get("cs_particle"));
      c_pr->SetCount(100);
      c_pr->Spread(-Vector3::One, Vector3::One);
      c_pr->SetMaterial(Resources::Material::Get("NormalSpecularSphere"));
      AddChild(child);
    }
  }

  inline TestCube::~TestCube() {}

  inline void TestCube::PreUpdate(const float& dt) { Object::PreUpdate(dt); }

  inline void TestCube::Update(const float& dt) { Object::Update(dt); }

  inline void TestCube::PreRender(const float& dt) { Object::PreRender(dt); }

  inline void TestCube::Render(const float& dt) { Object::Render(dt); }

  void TestCube::PostRender(const float& dt) { Object::PostRender(dt); }

  inline void TestCube::FixedUpdate(const float& dt) { Object::FixedUpdate(dt); }
} // namespace Client::Object
