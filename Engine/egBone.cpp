#include "pch.h"
#include "egBone.h"

#include "egDXAnimCommon.hpp"

SERIALIZE_IMPL
(
 Engine::Resources::Bone,
 _ARTAG(_BSTSUPER(Resource))
 _ARTAG(m_bone_map)
)

namespace Engine::Resources
{
	Bone::Bone(const BonePrimitiveMap& bone_map)
		: Resource("", RES_T_BONE),
		  m_bone_map(bone_map)
	{
		for (auto& bone : m_bone_map | std::views::values)
		{
			m_bones_index_wise_.push_back(&bone);
		}

		std::ranges::sort
				(
				 m_bones_index_wise_, [](const BonePrimitive* lhs, const BonePrimitive* rhs)
				 {
					 return lhs->GetIndex() < rhs->GetIndex();
				 }
				);
	}

	Bone::Bone(const Bone& other)
		: Resource("", RES_T_BONE)
	{
		m_bone_map = other.m_bone_map;

		for (auto& bone : m_bone_map | std::views::values)
		{
			m_bones_index_wise_.push_back(&bone);
		}

		std::ranges::sort
				(
				 m_bones_index_wise_, [](const BonePrimitive* lhs, const BonePrimitive* rhs)
				 {
					 return lhs->GetIndex() < rhs->GetIndex();
				 }
				);
	}

	Bone& Bone::operator=(Bone&& other) noexcept
	{
		m_bone_map = other.m_bone_map;

		for (auto& bone : m_bone_map | std::views::values)
		{
			m_bones_index_wise_.push_back(&bone);
		}

		std::ranges::sort
				(
				 m_bones_index_wise_, [](const BonePrimitive* lhs, const BonePrimitive* rhs)
				 {
					 return lhs->GetIndex() < rhs->GetIndex();
				 }
				);

		return *this;
	}

	void Bone::PreUpdate(const float& dt) {}

	void Bone::Update(const float& dt) {}

	void Bone::FixedUpdate(const float& dt) {}

	void Bone::PostUpdate(const float& dt) {}

	void Bone::OnSerialized()
	{
		Resource::OnSerialized();
	}

	void Bone::OnDeserialized()
	{
		Resource::OnDeserialized();

		m_bones_index_wise_.resize(m_bone_map.size());
		for (auto& bone : m_bone_map | std::views::values)
		{
			m_bones_index_wise_[bone.GetIndex()] = &bone;
		}
	}

	const BonePrimitive* Bone::GetBone(const UINT idx) const
	{
		if (m_bones_index_wise_.size() > idx)
		{
			return m_bones_index_wise_[idx];
		}

		return nullptr;
	}

	const BonePrimitive* Bone::GetBone(const std::string& name)
	{
		if (m_bone_map.contains(name))
		{
			return &m_bone_map.at(name);
		}

		return nullptr;
	}

	bool Bone::Contains(const std::string& name) const
	{
		return m_bone_map.contains(name);
	}

	const BonePrimitive* Bone::GetBoneParent(const UINT idx) const
	{
		const auto parent_idx = m_bones_index_wise_[idx]->GetParentIndex();
		if (parent_idx == -1)
		{
			return nullptr;
		}
		return m_bones_index_wise_[parent_idx];
	}

	size_t Bone::GetBoneCount() const
	{
		return m_bones_index_wise_.size();
	}

	void Bone::Load_INTERNAL() { }

	void Bone::Unload_INTERNAL() { }

	Bone::Bone()
		: Resource("", RES_T_BONE) {}
}
