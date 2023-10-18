#pragma once
#include <bitset>
#include "egCommon.hpp"
#include "egObject.hpp"
#include "egRenderPipeline.hpp"
#include "egTransform.hpp"

namespace Engine::Objects
{
	class Light final : public Abstract::Object
	{
	public:
		Light() = default;
		~Light() override;

		void SetColor(Vector4 color);
		void SetPosition(Vector3 position);

		void Initialize() override;
		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Render() override;

	private:
		UINT m_light_id_;
		Vector4 m_color_;
		inline static std::bitset<g_max_lights> s_light_map_{};
	};

	inline Light::~Light()
	{
		s_light_map_.reset(m_light_id_);
		Graphic::RenderPipeline::SetLightColor(m_light_id_, Vector4{0.0f, 0.0f, 0.0f, 1.0f});
		Graphic::RenderPipeline::SetLightPosition(m_light_id_, Vector3{0.0f, 0.0f, 0.0f});
	}

	inline void Light::SetColor(Vector4 color)
	{
		m_color_ = color;
	}

	inline void Light::SetPosition(Vector3 position)
	{
		const auto transform = GetComponent<Component::Transform>();
		transform.lock()->SetPosition(position);
	}

	inline void Light::Initialize()
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
		GetComponent<Component::Transform>().lock()->SetPosition(Vector3{1.0f, 0.0f, 0.0f});
		m_color_ = Vector4{1.0f, 1.0f, 1.0f, 1.0f};
	}

	inline void Light::PreUpdate()
	{
		Object::PreUpdate();
	}

	inline void Light::Update()
	{
		Object::Update();
	}

	inline void Light::PreRender()
	{
		Object::PreRender();
		Graphic::RenderPipeline::SetLightColor(m_light_id_, m_color_);
		Graphic::RenderPipeline::SetLightPosition(m_light_id_, GetComponent<Component::Transform>().lock()->GetPosition());
	}

	inline void Light::Render()
	{
		Object::Render();
	}
}
