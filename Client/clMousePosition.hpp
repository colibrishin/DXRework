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
	class MousePositionText : public Engine::Objects::Text
	{
	public:
		MousePositionText();
		void Initialize() override;
		~MousePositionText() override;

		inline void PreUpdate(const float& dt) override;
		inline void Update(const float& dt) override;
		inline void PreRender(const float dt) override;
		inline void Render(const float dt) override;
	};

	inline MousePositionText::MousePositionText() : Text(Engine::GetResourceManager().GetResource<Engine::Resources::Font>(L"DefaultFont"))
	{
	}

	inline void MousePositionText::Initialize()
	{
		SetText(L"X: 0, Y: 0");
		SetPosition(Vector2(0.0f, 32.0f));
		SetColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
		SetRotation(0.0f);
		SetScale(1.0f);
	}

	inline MousePositionText::~MousePositionText()
	{
	}

	inline void MousePositionText::PreUpdate(const float& dt)
	{
		Text::PreUpdate(dt);
	}

	inline void MousePositionText::Update(const float& dt)
	{
		Text::Update(dt);
		const Vector2 pos = Engine::GetSceneManager().GetActiveScene().lock()->GetMainCamera().lock()->GetWorldMousePosition();
		SetText(L"X: " + std::to_wstring(pos.x) + L", Y: " + std::to_wstring(pos.y));
	}

	inline void MousePositionText::PreRender(const float dt)
	{
		Text::PreRender(dt);
	}

	inline void MousePositionText::Render(const float dt)
	{
		Text::Render(dt);
	}
}
