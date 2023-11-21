#pragma once

#include "clTriangleMesh.hpp"
#include "../Engine/egCollider.hpp"
#include "../Engine/egManagerHelper.hpp"
#include "../Engine/egTexture.hpp"
#include "../Engine/egObject.hpp"
#include "../Engine/egResourceManager.hpp"
#include "../Engine/egTransform.hpp"
#include "../Engine/egIShader.hpp"
#include "../Engine/egRigidbody.hpp"
#include "../Engine/egText.hpp"

namespace Engine::Component
{
	class Rigidbody;
	class Transform;
}

namespace Client::Object
{
	class VelocityCounter : public Engine::Objects::Text
	{
	public:
		VelocityCounter();
		void Initialize() override;
		~VelocityCounter() override;

		inline void PreUpdate(const float& dt) override;
		inline void Update(const float& dt) override;
		inline void PreRender(const float dt) override;
		inline void Render(const float dt) override;

		void SetVelocity(const Vector3& velocity) { m_velocity_ = velocity; }

	private:
		Vector3 m_velocity_;
	};

	inline VelocityCounter::VelocityCounter() : Text(Engine::GetResourceManager().GetResource<Engine::Resources::Font>(L"DefaultFont"))
	{
	}

	inline void VelocityCounter::Initialize()
	{
		SetText(L"Velocity: 0");
		SetPosition(Vector2(0.0f, 64.0f));
		SetColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
		SetRotation(0.0f);
		SetScale(1.0f);
		SetLayer(Engine::LAYER_UI);
	}

	inline VelocityCounter::~VelocityCounter()
	{
	}

	inline void VelocityCounter::PreUpdate(const float& dt)
	{
		Text::PreUpdate(dt);
	}

	inline void VelocityCounter::Update(const float& dt)
	{
		Text::Update(dt);
		SetText(L"Velocity: " + std::to_wstring(m_velocity_.Length()));
	}

	inline void VelocityCounter::PreRender(const float dt)
	{
		Text::PreRender(dt);
	}

	inline void VelocityCounter::Render(const float dt)
	{
		Text::Render(dt);
	}
}
