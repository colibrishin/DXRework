#include "pch.h"
#include "egBone.h"

SERIALIZER_ACCESS_IMPL(
                       Engine::Resources::Bone,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Resource))
                       _ARTAG(m_primitive_))

namespace Engine::Resources
{
    Bone::Bone(BonePrimitive bones) : Resource("", RES_T_BONE), m_primitive_(std::move(bones)){}

    void Bone::PreUpdate(const float& dt) {}

    void Bone::Update(const float& dt) {}

    void Bone::FixedUpdate(const float& dt) {}

    void Bone::PreRender(const float& dt) {}

    void Bone::Render(const float& dt) {}

    void Bone::PostRender(const float& dt) {}

    void Bone::Load_INTERNAL() {}

    void Bone::Unload_INTERNAL() {}

    Bone::Bone()
    : Resource("", RES_T_BONE),
      m_primitive_() {}
}
