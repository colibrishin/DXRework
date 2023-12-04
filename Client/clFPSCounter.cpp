#include "pch.h"
#include "clFPSCounter.hpp"

CLIENT_OBJECT_IMPL(Client::Object::FPSCounter)

namespace Client::Object
{
	inline FPSCounter::FPSCounter() : Text(Engine::GetResourceManager().GetResource<Engine::Resources::Font>("DefaultFont"))
	{
	}

	inline void FPSCounter::Initialize()
	{
		SetText("FPS: 0");
		SetPosition(Vector2(0.0f, 0.0f));
		SetColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
		SetRotation(0.0f);
		SetScale(1.0f);
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
		SetText("FPS: " + std::to_string(Engine::GetApplication().GetFPS()));
	}

	inline void FPSCounter::PreRender(const float dt)
	{
		Text::PreRender(dt);
	}

	inline void FPSCounter::Render(const float dt)
	{
		Engine::Objects::Text::Render(dt);
	}
}
