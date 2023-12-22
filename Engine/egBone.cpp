#include "pch.h"
#include "egBone.h"

namespace Engine::Resources
{
    Bone::Bone(const BonePrimitiveMap& bone_map) : Resource("", RES_T_BONE), m_bone_map(bone_map)
    {
        for (const auto& bone : m_bone_map | std::views::values)
        {
            m_bones_index_wise_.push_back(bone);
        }

        std::ranges::sort(m_bones_index_wise_, [](const BonePrimitive& lhs, const BonePrimitive& rhs) { return lhs.idx < rhs.idx; });
    }

    void Bone::PreUpdate(const float& dt) {}

    void Bone::Update(const float& dt) {}

    void Bone::FixedUpdate(const float& dt) {}

    void Bone::PreRender(const float& dt) {}

    void Bone::Render(const float& dt)
    {
    }

    void Bone::PostRender(const float& dt) {}

    BonePrimitive Bone::GetBone(const UINT idx)
    {
        return m_bones_index_wise_[idx];
    }

    BonePrimitive Bone::GetBone(const std::string& name)
    {
        return m_bone_map.at(name);
    }

    UINT Bone::GetBoneCount() const
    {
        return m_bones_index_wise_.size();
    }

    void Bone::Load_INTERNAL()
    {
    }

    void Bone::Unload_INTERNAL()
    {
    }
}
