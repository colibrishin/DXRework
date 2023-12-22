#pragma once
#include "egResource.h"

namespace Engine::Resources
{
    class Bone : public Abstract::Resource
    {
    public:
        INTERNAL_RES_CHECK_CONSTEXPR(RES_T_BONE)

        Bone(const BonePrimitiveMap& bone_map);
        Bone(Bone&& other) noexcept = default;
        Bone(const Bone& other);
        Bone& operator=(Bone&& other) noexcept;

        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;

        [[nodiscard]] const BonePrimitive* GetBone(const UINT idx) const;
        [[nodiscard]] const BonePrimitive* GetBone(const std::string& name);
        [[nodiscard]] bool                 Contains(const std::string& name) const;
        [[nodiscard]] const BonePrimitive* GetBoneParent(const UINT idx) const;
        UINT                               GetBoneCount() const;

    protected:
        void Load_INTERNAL() override;
        void Unload_INTERNAL() override;

    private:
        BonePrimitiveMap           m_bone_map;
        std::vector<BonePrimitive*> m_bones_index_wise_;

    };
}
