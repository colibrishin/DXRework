#include "pch.h"
#include "egLight.h"
#include "egCamera.h"
#include "egManagerHelper.hpp"

SERIALIZER_ACCESS_IMPL(
                       Engine::Objects::Light,
                       _ARTAG(_BSTSUPER(Object)) _ARTAG(m_color_))

namespace Engine::Objects
{
    Light::Light(): Object(DEF_OBJ_T_LIGHT) {}

    Light::~Light() {}

    void Light::SetColor(Vector4 color)
    {
        m_color_ = color;
    }

    void Light::Initialize()
    {
        AddComponent<Components::Transform>();
        m_color_ = Vector4{1.0f, 1.0f, 1.0f, 1.0f};
        SetCulled(false);
    }

    void Light::PreUpdate(const float& dt)
    {
        Object::PreUpdate(dt);
    }

    void Light::Update(const float& dt)
    {
        Object::Update(dt);
    }

    void Light::PreRender(const float& dt)
    {
        Object::PreRender(dt);
    }

    void Light::Render(const float& dt)
    {
        Object::Render(dt);
#ifdef _DEBUG
        const auto tr = GetComponent<Components::Transform>().lock();

        const BoundingSphere sphere(tr->GetWorldPosition(), 0.5f);
        GetDebugger().Draw(sphere, DirectX::Colors::Yellow);
#endif
    }

    void Light::PostRender(const float& dt)
    {
        Object::PostRender(dt);
    }

    void Light::PostUpdate(const float& dt)
    {
        Object::PostUpdate(dt);
    }

    void Light::OnDeserialized()
    {
        Object::OnDeserialized();
    }
} // namespace Engine::Objects
