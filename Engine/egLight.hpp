#pragma once
#include "egCommon.hpp"
#include "egObject.hpp"
#include "egRenderPipeline.hpp"

namespace Engine::Objects
{
	class Light final : public Abstract::Object
	{
	public:
		Light() = default;
		~Light() override = default;

		void SetColor(Vector4 color);
		void SetDirection(Vector3 direction);

		void Initialize() override;
		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Render() override;

	private:
		LightBuffer m_light_buffer_;
	};

	inline void Light::Initialize()
	{
		m_light_buffer_.ambient = Vector4(0.15f, 0.15f, 0.15f, 1.0f);
		m_light_buffer_.color = Vector4{1.0f, 1.0f, 1.0f, 1.0f};
		m_light_buffer_.direction = Vector3{1.0f, 0.0f, 0.0f};
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
	}

	inline void Light::Render()
	{
		Object::Render();
		Graphic::RenderPipeline::SetLight(m_light_buffer_);
	}
}
