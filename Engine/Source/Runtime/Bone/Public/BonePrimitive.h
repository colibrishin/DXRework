#pragma once
#include <map>

#include "TypeLibrary.h"

#include "BonePrimitive.generated.h"

namespace Engine::Graphics 
{
	struct BonePrimitive;
	using BonePrimitiveMap = std::map<std::string, Graphics::BonePrimitive>;

	ECLASS(serialize)
	struct ENGINE_BONE_API BonePrimitive
	{
		GENERATE_BODY

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
		EPROPERTY()
		int    m_idx_;
		EPROPERTY()
		int    m_parent_idx_;
		EPROPERTY()
		Matrix m_inv_bind_pose_;
		EPROPERTY()
		Matrix m_transform_;
	};
}
