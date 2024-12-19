#pragma once
#include "Source/Runtime/Core/Resource/Public/Resource.h"
#include <map>

namespace Engine::Graphics 
{
	using BonePrimitiveMap = std::map<std::string, Graphics::BonePrimitive>;

	struct BONE_API BonePrimitive
	{
		BonePrimitive()
			: m_idx_(0),
			m_parent_idx_(-1) { }

		BonePrimitive(BonePrimitive&& other) noexcept
		{
			m_idx_ = other.m_idx_;
			m_parent_idx_ = other.m_parent_idx_;
			m_inv_bind_pose_ = other.m_inv_bind_pose_;
			m_transform_ = other.m_transform_;
		}

		BonePrimitive(const BonePrimitive& other) noexcept
		{
			m_idx_ = other.m_idx_;
			m_parent_idx_ = other.m_parent_idx_;
			m_inv_bind_pose_ = other.m_inv_bind_pose_;
			m_transform_ = other.m_transform_;
		}

		BonePrimitive& operator=(const BonePrimitive& other) noexcept = default;

		__forceinline void SetIndex(const int idx) noexcept
		{
			m_idx_ = idx;
		}

		__forceinline void SetParentIndex(const int idx) noexcept
		{
			m_parent_idx_ = idx;
		}

		__forceinline void SetInvBindPose(const Matrix& inv_bind_pose) noexcept
		{
			m_inv_bind_pose_ = inv_bind_pose;
		}

		__forceinline void SetTransform(const Matrix& transform) noexcept
		{
			m_transform_ = transform;
		}

		__forceinline int GetIndex() const noexcept
		{
			return m_idx_;
		}

		__forceinline int GetParentIndex() const noexcept
		{
			return m_parent_idx_;
		}

		__forceinline const Matrix& GetInvBindPose() const noexcept
		{
			return m_inv_bind_pose_;
		}

		__forceinline const Matrix& GetTransform() const noexcept
		{
			return m_transform_;
		}

	private:
		int    m_idx_;
		int    m_parent_idx_;
		Matrix m_inv_bind_pose_;
		Matrix m_transform_;
	};
}

namespace Engine::Resources
{
	using namespace Graphics;

	class BONE_API Bone : public Abstracts::Resource
	{
	public:
		RESOURCE_T(RES_T_BONE)

		Bone(const BonePrimitiveMap& bone_map);
		Bone(Bone&& other) noexcept = default;
		Bone(const Bone& other);
		Bone& operator=(Bone&& other) noexcept;

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostUpdate(const float& dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		[[nodiscard]] const BonePrimitive* GetBone(UINT idx) const;
		[[nodiscard]] const BonePrimitive* GetBone(const std::string& name);
		[[nodiscard]] bool                 Contains(const std::string& name) const;
		[[nodiscard]] const BonePrimitive* GetBoneParent(UINT idx) const;
		size_t                             GetBoneCount() const;

		RESOURCE_SELF_INFER_GETTER_DECL(Bone)

	protected:
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		Bone();

		BonePrimitiveMap            m_bone_map;
		std::vector<BonePrimitive*> m_bones_index_wise_;
	};
}

