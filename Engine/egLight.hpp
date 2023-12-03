#pragma once
#include <bitset>

#include "egCommon.hpp"
#include "egObject.hpp"
#include "egRenderPipeline.hpp"
#include "egTransform.hpp"
#include "egObject.hpp"

namespace Engine::Objects
{
	class Light final : public Abstract::Object
	{
	public:
		Light() : Object(), m_light_id_(0)
		{
		}

		~Light() override;

		void SetColor(Vector4 color);
		void SetPosition(Vector3 position);

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;

	private:
		void AfterDeserialized() override;

		friend class boost::serialization::access;
		UINT m_light_id_;
		Vector4 m_color_;
		inline static std::bitset<g_max_lights> s_light_map_{};
	};

	inline Light::~Light()
	{
		s_light_map_.reset(m_light_id_);
		GetRenderPipeline().SetLightColor(m_light_id_, Vector4{0.0f, 0.0f, 0.0f, 1.0f});
		GetRenderPipeline().SetLightPosition(m_light_id_, Vector3{0.0f, 0.0f, 0.0f});
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
		m_color_ = Vector4{1.0f, 1.0f, 1.0f, 1.0f};
	}

	inline void Light::PreUpdate(const float& dt)
	{
		Object::PreUpdate(dt);
	}

	inline void Light::Update(const float& dt)
	{
		Object::Update(dt);
	}

	inline void Light::PreRender(const float dt)
	{
		Object::PreRender(dt);
		GetRenderPipeline().SetLightColor(m_light_id_, m_color_);
		GetRenderPipeline().SetLightPosition(m_light_id_, GetComponent<Component::Transform>().lock()->GetPosition());
	}

	inline void Light::Render(const float dt)
	{
		Object::Render(dt);
	}

	inline void Light::AfterDeserialized()
	{
		Object::AfterDeserialized();
		// @todo: use same light id?
	}
}
