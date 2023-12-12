#include "pch.hpp"
#include "egLight.hpp"
#include "egCamera.hpp"
#include "egManagerHelper.hpp"

SERIALIZER_ACCESS_IMPL(
	Engine::Objects::Light,
	_ARTAG(_BSTSUPER(Object))
	_ARTAG(m_color_))

namespace Engine::Objects
{
	Light::~Light()
	{
	}

	void Light::SetColor(Vector4 color)
	{
		m_color_ = color;
	}

	void Light::SetPosition(Vector3 position)
	{
		const auto transform = GetComponent<Component::Transform>();
		transform.lock()->SetPosition(position);
	}

	void Light::Initialize()
	{
		AddComponent<Component::Transform>();
		m_color_ = Vector4{1.0f, 1.0f, 1.0f, 1.0f};
		SetCulled(false);
	}

	void Light::PreUpdate(const float& dt)
	{
		Object::PreUpdate(dt);
	}

	void Light::Update(const float& dt)
	{
		Object::Update(dt);
	}

	void Light::PreRender(const float dt)
	{
		Object::PreRender(dt);
	}

	void Light::Render(const float dt)
	{
		Object::Render(dt);
#ifdef _DEBUG
		const auto tr = GetComponent<Component::Transform>().lock();

		const BoundingSphere sphere (tr->GetPosition(), 0.5f);
		GetDebugger().Draw(sphere, Colors::Yellow);
#endif
	}

	void Light::OnDeserialized()
	{
		Object::OnDeserialized();
	}
}