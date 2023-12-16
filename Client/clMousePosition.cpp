#include "pch.h"
#include "clMousePosition.hpp"
#include <egCamera.h>

SERIALIZER_ACCESS_IMPL(
                       Client::Object::MousePositionText,
                       _ARTAG(_BSTSUPER(Text)))

namespace Client::Object
{
    MousePositionText::MousePositionText()
    : Text(
           Engine::GetResourceManager().GetResource<Engine::Resources::Font>(
                                                                             "DefaultFont")) {}

    void MousePositionText::Initialize()
    {
        SetText("X: 0, Y: 0");
        SetPosition(Vector2(0.0f, 32.0f));
        SetColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
        SetRotation(0.0f);
        SetScale(1.0f);
    }

    MousePositionText::~MousePositionText() {}

    void MousePositionText::PreUpdate(const float& dt)
    {
        Text::PreUpdate(dt);
    }

    void MousePositionText::Update(const float& dt)
    {
        Text::Update(dt);
        const Vector2 pos = Engine::GetSceneManager()
                            .GetActiveScene()
                            .lock()
                            ->GetMainCamera()
                            .lock()
                            ->GetWorldMousePosition();
        SetText("X: " + std::to_string(pos.x) + ", Y: " + std::to_string(pos.y));
    }

    void MousePositionText::PreRender(const float& dt)
    {
        Text::PreRender(dt);
    }

    void MousePositionText::Render(const float& dt)
    {
        Text::Render(dt);
    }

    void MousePositionText::PostRender(const float& dt)
    {
        Text::PostRender(dt);
    }
} // namespace Client::Object
