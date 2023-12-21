#pragma once
#include "egCommon.hpp"
#include "egResource.h"
#include "egResourceManager.hpp"

namespace Engine::Resources
{
    class Bone : public Abstract::Resource
    {
    public:
        INTERNAL_RES_CHECK_CONSTEXPR(RES_T_BONE)
        Bone(BonePrimitive bones);

        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;

        RESOURCE_SELF_INFER_GETTER(Bone)

    protected:
        void Load_INTERNAL() override;
        void Unload_INTERNAL() override;

    private:
        Bone();
        SERIALIZER_ACCESS

        BonePrimitive m_primitive_;
    };
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Bone)