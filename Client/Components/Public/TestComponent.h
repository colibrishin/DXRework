#pragma once
#include "Component/Public/Component.h"
#include "TestComponent.generated.h"

ECLASS(component=client)
class ENGINE_CLIENT_API TestComponent : public Engine::Abstracts::Component
{
	GENERATE_BODY
public:
	TestComponent(const Engine::Weak<Engine::Abstracts::ObjectBase>& owner) : Base(owner) {};

	void PreUpdate(const float dt) override;
	void Update(const float dt) override;
	void PostUpdate(const float dt) override;
	void FixedUpdate(const float dt) override;
	void OnSerialized() override;
	void OnDeserialized() override;

	Engine::eComponentUpdatePriorities GetUpdatePriority() const override;
};