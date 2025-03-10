#include "AnimationTexture.h"
#include "AnimationTexture.generated.h"

#include <algorithm>

#include "BoneAnimation.h"
#include "ResourceManager.h"

namespace Engine::Resources
{
	AnimationTexture::AnimationTexture(const std::vector<Strong<BoneAnimation>>& animations)
		: Texture3D("", {}),
		  m_animations_(animations) 
	{
		for (const Strong<BoneAnimation>& animation : m_animations_)
		{
			m_cached_animations_.push_back(animation);
		}
	}

	void AnimationTexture::PreUpdate(const float dt) {}

	void AnimationTexture::Update(const float dt) {}

	void AnimationTexture::FixedUpdate(const float dt) {}

	void AnimationTexture::PostUpdate(const float dt) {}

	void AnimationTexture::OnSerialized()
	{
		Texture3D::OnSerialized();

		m_animations_meta_path_.clear();
		for (const Weak<BoneAnimation>& anim : m_cached_animations_)
		{
			if (const Strong<BoneAnimation>& locked = anim.lock())
			{
				Serializer::Serialize(locked->GetName(), locked);
				m_animations_meta_path_.emplace_back(locked->GetMetadataPath());
			}
		}
	}

	void AnimationTexture::OnDeserialized()
	{
		Texture3D::OnDeserialized();

		for (const std::filesystem::path& meta_path : m_animations_meta_path_)
		{
			if (const Strong<BoneAnimation>& anim = Resources::BoneAnimation::GetByMetadataPath(meta_path).lock())
			{
				if (IsLoaded())
				{
					m_animations_.emplace_back(anim);
				}

				m_cached_animations_.emplace_back(anim);
			}
		}
	}

	void AnimationTexture::Load_INTERNAL()
	{
		const GenericTextureDescription& new_desc = preEvaluateAnimations(m_animations_, m_evaluated_animations_);
		UpdateDescription(new_desc);

		Texture3D::Load_INTERNAL();
	}

	void AnimationTexture::Unload_INTERNAL()
	{
		Texture3D::Load_INTERNAL();
		
		m_animations_.clear();
	}

	void AnimationTexture::Map()
	{
		Texture3D::Map();

		const GenericTextureDescription& desc = GetDescription();
		ITexture* tex = GetPrimitiveTexture();

		tex->Map(
			m_evaluated_animations_.data(),
			desc.Width / s_vec4_to_mat,
			desc.Height,
			sizeof(Matrix),
			desc.DepthOrArraySize);
	}


	const std::vector<Weak<BoneAnimation>>& AnimationTexture::GetAnimations() const
	{
		return m_cached_animations_;
	}

	Weak<BoneAnimation> AnimationTexture::GetAnimation(const size_t idx) const
	{
		if (m_cached_animations_.size() > idx)
		{
			return m_cached_animations_.at(idx);
		}

		return {};
	}

	GenericTextureDescription AnimationTexture::preEvaluateAnimations(
		const std::vector<Strong<BoneAnimation>>& anims, std::vector<std::vector<std::vector<Matrix>>>& preEvaluated
	)
	{
		const UINT anim_count  = static_cast<UINT>(anims.size());
		UINT       frame_count = 0;
		UINT       bone_count  = 0;

		for (const auto& animation : anims)
		{
			std::vector<std::vector<Matrix>> sample_data;
			float                            t = 0.f;

			for (; t < animation->GetDuration() / animation->GetTicksPerSecond();
			       t += 1.f / (animation->GetTicksPerSecond() + 0.99999f))
			{
				// Transpose will be done while loading the texture to the shader.
				auto bones = animation->GetFrameAnimationDt(t);
				bone_count = std::max(bone_count, static_cast<UINT>(bones.size()));

				sample_data.push_back(bones);
			}

			preEvaluated.push_back(sample_data);
			frame_count = std::max(frame_count, static_cast<UINT>(sample_data.size()));
		}

		return
		{
			.Dimension = TEX_TYPE_3D,
			.Alignment = 0,
			.Width = static_cast<UINT>(bone_count * s_vec4_to_mat),
			.Height = frame_count,
			.DepthOrArraySize = static_cast<UINT16>(anim_count),
			.Format = TEX_FORMAT_R32G32B32A32_FLOAT,
			.Flags = RESOURCE_FLAG_NONE,
			.MipsLevel = 1,
			.Layout = TEX_LAYOUT_UNKNOWN,
			.SampleDesc = {.Count = 1, .Quality = 0}
		};
	}
}
