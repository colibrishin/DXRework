#include "Components/Public/HitboxComponent.h"

void HitboxComponent::Hit(const float dmg) const
{
}

void HitboxComponent::PreUpdate(const float dt)
{
}

void HitboxComponent::PostUpdate(const float dt)
{
}

void HitboxComponent::Update(const float dt)
{
}

void HitboxComponent::FixedUpdate(const float dt)
{
}

void HitboxComponent::OnSerialized()
{
}

void HitboxComponent::OnDeserialized()
{
}

Engine::eComponentUpdatePriorities HitboxComponent::GetUpdatePriority() const
{
	return Engine::eComponentUpdatePriority::COM_PRIORITY_PHYSICS;
}
