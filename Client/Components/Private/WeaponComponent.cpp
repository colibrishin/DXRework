#include "Components/Public/WeaponComponent.h"
#include "WeaponComponent.generated.h"

#include "Object.h"
#include "Shape.h"
#include "ModelRenderer.h"
#include "Animator.h"
#include "Transform.h"
#include "Collider.h"

void WeaponComponent::Initialize()
{
	Component::Initialize();

	const auto obj = GetOwner().lock();
	const auto rifle = obj->GetScene().lock()->CreateGameObject<Engine::Object>(GetOwner().lock()->GetLayer()).lock();
	obj->AddChild(rifle);

	const auto rifle_model = Engine::Resources::Shape::Get("RifleShape").lock();

	const auto cmr = rifle->AddComponent<Engine::Components::ModelRenderer>().lock();

	cmr->SetShape(rifle_model);

	const auto ctr = rifle->AddComponent<Engine::Components::Transform>().lock();
	const auto catr = rifle->AddComponent<Engine::Components::Animator>().lock();
	const auto ccldr = rifle->AddComponent<Engine::Components::Collider>().lock();

	ctr->SetSizeAbsolute(true);
	ctr->SetRotateAbsolute(false);
	catr->SetAnimation(0);
	Engine::ShapeExtension::BindShapeToCollider(rifle_model, ccldr);
}

void WeaponComponent::PreUpdate(const float dt)
{
}

void WeaponComponent::PostUpdate(const float dt)
{
}

void WeaponComponent::Update(const float dt)
{
}

void WeaponComponent::FixedUpdate(const float dt)
{
}

void WeaponComponent::OnSerialized()
{
}

void WeaponComponent::OnDeserialized()
{
}

float WeaponComponent::GetFireRate() const
{
    return m_fire_rate_;
}

WeaponComponent::WeaponComponent(const Engine::Weak<Engine::Abstracts::ObjectBase>& owner) : 
	Component(owner),
	m_fire_rate_(3.f)
{
}

Engine::eComponentUpdatePriorities WeaponComponent::GetUpdatePriority() const
{
	return Engine::eComponentUpdatePriority::COM_PRIORITY_POSITIONAL;
}
