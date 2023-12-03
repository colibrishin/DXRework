#include "pch.hpp"
#include "egLight.hpp"

SERIALIZER_ACCESS_IMPL(
	Engine::Objects::Light,
	_ARTAG(_BSTSUPER(Object))
	_ARTAG(m_light_id_)
	_ARTAG(m_color_))

namespace Engine::Objects
{
	Light::~Light()
	{
		s_light_map_.reset(m_light_id_);
		GetRenderPipeline().SetLightColor(m_light_id_, Vector4{0.0f, 0.0f, 0.0f, 1.0f});
		GetRenderPipeline().SetLightPosition(m_light_id_, Vector3{0.0f, 0.0f, 0.0f});
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
		assert(s_light_map_.count() < g_max_lights);

		for (int i = 0; i < g_max_lights; ++i)
		{
			if (!s_light_map_[i])
			{
				m_light_id_ = i;
				s_light_map_.set(i);
				break;
			}
		}

		AddComponent<Component::Transform>();
		m_color_ = Vector4{1.0f, 1.0f, 1.0f, 1.0f};
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
		GetRenderPipeline().SetLightColor(m_light_id_, m_color_);
		GetRenderPipeline().SetLightPosition(m_light_id_, GetComponent<Component::Transform>().lock()->GetPosition());
	}

	void Light::Render(const float dt)
	{
		Object::Render(dt);
	}

	void Light::OnDeserialized()
	{
		Object::OnDeserialized();
	}
}