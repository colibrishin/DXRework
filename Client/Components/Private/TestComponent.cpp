#include "Components/Public/TestComponent.h"
#include "TestComponent.generated.h"

void TestComponent::PreUpdate(const float dt)
{
}

void TestComponent::Update(const float dt)
{
}

void TestComponent::PostUpdate(const float dt)
{
}

void TestComponent::FixedUpdate(const float dt)
{
}

void TestComponent::OnSerialized()
{
}

void TestComponent::OnDeserialized()
{
}

Engine::eComponentUpdatePriorities TestComponent::GetUpdatePriority() const
{
	return 0;
}
