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

		inline void PreUpdate() override;
		inline void Update() override;
		inline void PreRender() override;
		inline void Render() override;
	};

	inline MousePositionText::MousePositionText() : Text(Engine::GetResourceManager()->GetResource<Engine::Resources::Font>(L"DefaultFont"))
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

	inline void MousePositionText::PreUpdate()
	{
		Text::PreUpdate();
	}

	inline void MousePositionText::Update()
	{
		Text::Update();
		SetText(L"X: " + std::to_wstring(Engine::Application::GetMouseState().x) + L", Y: " + std::to_wstring(Engine::Application::GetMouseState().y));
	}

	inline void MousePositionText::PreRender()
	{
		Text::PreRender();
	}

	inline void MousePositionText::Render()
	{
		Text::Render();
	}
}
