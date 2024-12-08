#include "../Public/Light.h"

#include "Source/Runtime/Core/Components/Transform/Public/Transform.h"

namespace Engine::Objects
{
	OBJ_CLONE_IMPL(Light)

	Light::Light()
		: ObjectBase(DEF_OBJ_T_LIGHT),
		  m_radius_(0.5f),
		  m_range_(10.f),
		  m_type_(LIGHT_T_DIRECTIONAL) {}

	Light::~Light() {}

	void Light::SetColor(Vector4 color)
	{
		m_color_ = color;
	}

	void Light::SetType(eLightType type)
	{
		m_type_ = type;

		if (m_type_ == LIGHT_T_DIRECTIONAL)
		{
			SetRange(0.0f);
		}
		else
		{
			SetRange(10.0f);
		}
	}

	void Light::SetRange(float range)
	{
		if (m_type_ == LIGHT_T_DIRECTIONAL)
		{
			return;
		}

		m_range_ = range;
	}

	void Light::Initialize()
	{
		AddComponent<Components::Transform>();
		m_color_ = Vector4{1.0f, 1.0f, 1.0f, 1.0f};
		SetCulled(false);
	}

	void Light::PreUpdate(const float& dt)
	{
		ObjectBase::PreUpdate(dt);
	}

	void Light::Update(const float& dt)
	{
		ObjectBase::Update(dt);
	}

	void Light::PreRender(const float& dt)
	{
		ObjectBase::PreRender(dt);
	}

	void Light::Render(const float& dt)
	{
		ObjectBase::Render(dt);
	}

	void Light::PostRender(const float& dt)
	{
		ObjectBase::PostRender(dt);
	}

	void Light::PostUpdate(const float& dt)
	{
		ObjectBase::PostUpdate(dt);
	}

	void Light::OnDeserialized()
	{
		ObjectBase::OnDeserialized();
	}
} // namespace Engine::Objects