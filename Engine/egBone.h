#pragma once
#include "egResource.h"

namespace Engine::Resources
{
    class Bone : public Abstract::Resource
    {
    public:
        INTERNAL_RES_CHECK_CONSTEXPR(RES_T_BONE)

        Bone(const BonePrimitiveMap& bone_map);

        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;

        BonePrimitive GetBone(const UINT idx);
        BonePrimitive GetBone(const std::string& name);
        UINT GetBoneCount() const;

    protected:
        void Load_INTERNAL() override;
        void Unload_INTERNAL() override;

    private:
        BonePrimitiveMap           m_bone_map;
        std::vector<BonePrimitive> m_bones_index_wise_;

    };
}
