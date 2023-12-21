#include "pch.h"
#include "egAnimation.h"

SERIALIZER_ACCESS_IMPL(Engine::Resources::Animation,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Resource))
                       _ARTAG(m_primitive_))

namespace Engine::Resources
{
    Animation::Animation(const AnimationPrimitive& anim) : Resource("", RES_T_ANIM), m_primitive_(anim) {}

    void Animation::PreUpdate(const float& dt) {}

    void Animation::Update(const float& dt) {}

    void Animation::FixedUpdate(const float& dt) {}

    void Animation::PreRender(const float& dt) {}

    void Animation::Render(const float& dt) {}

    void Animation::PostRender(const float& dt) {}

    Animation::Animation() : Resource("", RES_T_ANIM) {}

    void Animation::Load_INTERNAL() {}

    void Animation::Unload_INTERNAL() {}
}
