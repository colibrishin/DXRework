#include "pch.h"
#include "clFPSCounter.hpp"

SERIALIZER_ACCESS_IMPL(Client::Object::FPSCounter, _ARTAG(_BSTSUPER(Text)))

namespace Client::Object
{
    FPSCounter::FPSCounter()
    : Text(
           Engine::GetResourceManager().GetResource<Engine::Resources::Font>(
                                                                             "DefaultFont")) {}

    void FPSCounter::Initialize()
    {
        SetText("FPS: 0");
        SetPosition(Vector2(0.0f, 0.0f));
        SetColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
        SetRotation(0.0f);
        SetScale(1.0f);
    }

    FPSCounter::~FPSCounter() {}

    void FPSCounter::PreUpdate(const float& dt)
    {
        Text::PreUpdate(dt);
    }

    void FPSCounter::Update(const float& dt)
    {
        Text::Update(dt);
        SetText("FPS: " + std::to_string(Engine::GetApplication().GetFPS()));
    }

    void FPSCounter::PreRender(const float& dt)
    {
        Text::PreRender(dt);
    }

    void FPSCounter::Render(const float& dt)
    {
        Text::Render(dt);
    }

    void FPSCounter::PostRender(const float& dt)
    {
        Text::PostRender(dt);
    }
} // namespace Client::Object
