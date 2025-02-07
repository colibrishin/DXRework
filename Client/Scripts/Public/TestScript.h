#pragma once
#include "Script/Public/Script.h"

#include "TestScript.generated.h"

ECLASS(script)
class ENGINE_CLIENT_API TestScript : public Engine::Script
{
	GENERATE_BODY
public:
	TestScript(const Engine::Weak<Engine::Abstracts::ObjectBase>& owner) : Base(owner) {};

	void PreUpdate(const float dt) override;
	void Update(const float dt) override;
	void PostUpdate(const float dt) override;
	void FixedUpdate(const float dt) override;
	void OnSerialized() override;
	void OnDeserialized() override;
	void PreRender(const float dt) override;
	void Render(const float dt) override;
	void PostRender(const float dt) override;
	void OnCollisionEnter(const Engine::Weak<Engine::Components::Collider>& other) override;
	void OnCollisionContinue(const Engine::Weak<Engine::Components::Collider>& other) override;
	void OnCollisionExit(const Engine::Weak<Engine::Components::Collider>& other) override;

};