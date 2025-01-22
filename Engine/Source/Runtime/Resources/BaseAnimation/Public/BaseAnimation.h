#pragma once
#include "Source/Runtime/Abstracts/CoreResource/Public/Resource.h"
#include "Source/Runtime/Managers/ResourceManager/Public/ResourceManager.hpp"

namespace Engine::Graphics 
{
	struct BoneAnimationPrimitive
	{
	public:
		BoneAnimationPrimitive()
			: bone_idx(0) { }

		__forceinline void SetIndex(const int idx) noexcept
		{
			bone_idx = idx;
		}

		__forceinline void AddPosition(const float time, const Vector3& position)
		{
			m_positions_.emplace_back(time, position);
		}

		__forceinline void AddScale(const float time, const Vector3& scale)
		{
			m_scales_.emplace_back(time, scale);
		}

		__forceinline void AddRotation(const float time, const Quaternion& rotation)
		{
			m_rotations_.emplace_back(time, rotation);
		}

		__forceinline int GetIndex() const noexcept
		{
			return bone_idx;
		}

		Vector3 GetPosition(const float time) const
		{
			if (m_positions_.size() == 1)
			{
				return m_positions_[0].second;
			}

			for (int i = 0; i < m_positions_.size() - 1; ++i)
			{
				if (time < m_positions_[i + 1].first)
				{
					const auto& p0 = m_positions_[i];
					const auto& p1 = m_positions_[i + 1];

					const auto t = (time - p0.first) / (p1.first - p0.first);

					return Vector3::Lerp(p0.second, p1.second, t);
				}
			}

			return m_positions_.back().second;
		}

		Vector3 GetScale(const float time) const
		{
			if (m_scales_.size() == 1)
			{
				return m_scales_[0].second;
			}

			for (size_t i = 0; i < m_scales_.size() - 1; ++i)
			{
				if (time < m_scales_[i + 1].first)
				{
					const auto& p0 = m_scales_[i];
					const auto& p1 = m_scales_[i + 1];

					const auto t = (time - p0.first) / (p1.first - p0.first);

					return Vector3::Lerp(p0.second, p1.second, t);
				}
			}

			return m_scales_.back().second;
		}

		Quaternion GetRotation(const float time) const
		{
			for (size_t i = 0; i < m_rotations_.size() - 1; ++i)
			{
				if (time < m_rotations_[i + 1].first)
				{
					const auto& p0 = m_rotations_[i];
					const auto& p1 = m_rotations_[i + 1];

					const auto t = (time - p0.first) / (p1.first - p0.first);
					const auto lerp = Quaternion::Slerp(p0.second, p1.second, t);
					Quaternion norm;
					lerp.Normalize(norm);

					return norm;
				}
			}

			return m_rotations_.back().second;
		}

	private:
		friend class boost::serialization::access;

		template <class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar& bone_idx;
			ar& m_positions_;
			ar& m_scales_;
			ar& m_rotations_;
		}

		int                                       bone_idx;
		std::vector<std::pair<float, Vector3>>    m_positions_;
		std::vector<std::pair<float, Vector3>>    m_scales_;
		std::vector<std::pair<float, Quaternion>> m_rotations_;
	};
}

namespace Engine::Resources
{
	using namespace Graphics;

	class BaseAnimation : public Abstracts::Resource
	{
	public:
		RESOURCE_T(RES_T_BASE_ANIM)

		BaseAnimation(const BoneAnimationPrimitive& primitive);

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostUpdate(const float& dt) override;

		void OnDeserialized() override;
		void OnSerialized() override;

		void  SetTicksPerSecond(const float& ticks_per_second);
		void  SetDuration(const float& duration);
		float GetTicksPerSecond() const;
		float GetDuration() const;

		static float ConvertDtToFrame(const float& dt, float ticks_per_second);

		RESOURCE_SELF_INFER_GETTER_DECL(BaseAnimation)

		static boost::shared_ptr<BaseAnimation> Create(
			const std::string& name, const BoneAnimationPrimitive& primitive
		)
		{
			if (const auto check = Managers::ResourceManager::GetInstance().GetResource<BaseAnimation>(name).lock())
			{
				return check;
			}
			const auto obj = boost::make_shared<BaseAnimation>(primitive);
			Managers::ResourceManager::GetInstance().AddResource(name, obj);
			return obj;
		}

	protected:
		friend class Components::Animator;

		SERIALIZE_DECL

		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

		BaseAnimation();

		float m_ticks_per_second_;
		float m_duration_;

		BoneAnimationPrimitive m_simple_primitive_;
	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::BaseAnimation)
BOOST_CLASS_EXPORT_KEY(Engine::Graphics::BoneAnimationPrimitive)