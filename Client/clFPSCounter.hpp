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
	class FPSCounter : public Engine::Objects::Text
	{
	public:
		FPSCounter();
		void Initialize() override;
		~FPSCounter() override;

		inline void PreUpdate(const float& dt) override;
		inline void Update(const float& dt) override;
		inline void PreRender(const float dt) override;
		inline void Render(const float dt) override;
	};

	inline FPSCounter::FPSCounter() : Text(Engine::GetResourceManager().GetResource<Engine::Resources::Font>(L"DefaultFont"))
	{
	}

	inline void FPSCounter::Initialize()
	{
		SetText(L"FPS: 0");
		SetPosition(Vector2(0.0f, 0.0f));
		SetColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
		SetRotation(0.0f);
		SetScale(1.0f);
		SetLayer(Engine::LAYER_UI);
	}

	inline FPSCounter::~FPSCounter()
	{
	}

	inline void FPSCounter::PreUpdate(const float& dt)
	{
		Text::PreUpdate(dt);
	}

	inline void FPSCounter::Update(const float& dt)
	{
		Text::Update(dt);
		SetText(L"FPS: " + std::to_wstring(Engine::GetApplication().GetFPS()));
	}

	inline void FPSCounter::PreRender(const float dt)
	{
		Text::PreRender(dt);
	}

	inline void FPSCounter::Render(const float dt)
	{
		Text::Render(dt);
	}
}
