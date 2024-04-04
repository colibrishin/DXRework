#include "pch.h"
#include "clRifleScript.h"

#include <egObject.hpp>
#include <egShape.h>
#include <egTransform.h>
#include <egModelRenderer.h>
#include <egMaterial.h>
#include <egAnimator.h>
#include <egBaseCollider.hpp>

SERIALIZE_IMPL
(
  Client::Scripts::RifleScript,
  _ARTAG(_BSTSUPER(Engine::Script))
  _ARTAG(m_fire_rate_)
)

namespace Client::Scripts
{
  SCRIPT_CLONE_IMPL(RifleScript)

  RifleScript::RifleScript(const WeakObjectBase& owner)
    : Script(SCRIPT_T_RIFLE, owner),
      m_fire_rate_(.3f) {}

  void RifleScript::Initialize()
  {
    Script::Initialize();

    const auto obj = GetOwner().lock();
    const auto rifle = obj->GetScene().lock()->CreateGameObject<Object>(GetOwner().lock()->GetLayer()).lock();
    obj->AddChild(rifle);

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
  }

  void RifleScript::PreUpdate(const float& dt) {}

  void RifleScript::Update(const float& dt) {}

  void RifleScript::FixedUpdate(const float& dt) {}

  void RifleScript::PreRender(const float& dt) {}

  void RifleScript::Render(const float& dt) {}

  void RifleScript::PostRender(const float& dt) {}

  void RifleScript::PostUpdate(const float& dt) {}

  void RifleScript::OnImGui() {}

  float RifleScript::GetFireRate() const { return m_fire_rate_; }

  void RifleScript::OnCollisionEnter(const WeakCollider& other) {}

  void RifleScript::OnCollisionContinue(const WeakCollider& other) {}

  void RifleScript::OnCollisionExit(const WeakCollider& other) {}

  RifleScript::RifleScript()
    : Script(SCRIPT_T_RIFLE, {}),
      m_fire_rate_(.3f) {}
}
