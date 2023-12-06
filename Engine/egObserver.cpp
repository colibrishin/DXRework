#include "pch.hpp"
#include "egObserver.hpp"
#include "egCollider.hpp"
#include "egTransform.hpp"
#include "egRigidbody.hpp"
#include "egObserverController.hpp"

SERIALIZER_ACCESS_IMPL(Engine::Objects::Observer,
	_ARTAG(_BSTSUPER(Engine::Abstract::Object)))

namespace Engine::Objects
{
	void Observer::Initialize()
	{
		Object::Initialize();

		AddComponent<Component::Transform>();
		AddComponent<Component::ObserverController>();
	}

	Observer::~Observer()
	{
	}

	void Observer::PreUpdate(const float& dt)
	{
		Object::PreUpdate(dt);
	}

	void Observer::Update(const float& dt)
	{
		Object::Update(dt);
	}

	void Observer::PreRender(const float dt)
	{
		Object::PreRender(dt);
	}

	void Observer::Render(const float dt)
	{
		Object::Render(dt);
	}

	void Observer::FixedUpdate(const float& dt)
	{
		Object::FixedUpdate(dt);
	}

	void Observer::OnImGui()
	{
		Object::OnImGui();
	}

	void Observer::OnDeserialized()
	{
		return;
	}
}
