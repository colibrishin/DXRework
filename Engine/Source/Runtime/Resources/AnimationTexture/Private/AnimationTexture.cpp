#include "../Public/AnimationTexture.h"

#include <algorithm>

#include "Source/Runtime/Resources/BoneAnimation/Public/BoneAnimation.h"
#include "Source/Runtime/Core/SIMDExtension/Public/SIMDExtension.hpp"
#include "Source/Runtime/Managers/ResourceManager/Public/ResourceManager.hpp"

namespace Engine::Resources
{
	AnimationTexture::AnimationTexture(const std::vector<Strong<BoneAnimation>>& animations)
		: Texture3D("", {}),
		  m_animations_(animations) {}

	void AnimationTexture::PreUpdate(const float& dt) {}

	void AnimationTexture::Update(const float& dt) {}

	void AnimationTexture::FixedUpdate(const float& dt) {}

	void AnimationTexture::PostUpdate(const float& dt) {}

	void AnimationTexture::OnSerialized()
	{
		Texture3D::OnSerialized();
	}

	void AnimationTexture::OnDeserialized()
	{
		Texture3D::OnDeserialized();
	}

	eResourceType AnimationTexture::GetResourceType() const
	{
		return RES_T_ANIMS_TEX;
	}

	boost::shared_ptr<AnimationTexture> AnimationTexture::Create(
		const std::string& name, const std::vector<Strong<BoneAnimation>>& anims
	) {
		if (const auto ncheck = Managers::ResourceManager::GetInstance().GetResource<AnimationTexture>(name).lock())
		{
			return ncheck;
		}

		const auto obj = boost::make_shared<AnimationTexture>(anims);
		Managers::ResourceManager::GetInstance().AddResource(name, obj);
		return obj;
	}

	void AnimationTexture::Load_INTERNAL()
	{
		GenericTextureDescription new_desc = preEvaluateAnimations(m_animations_, m_evaluated_animations_);
		GetPrimitiveTexture()->UpdateDescription(GetSharedPtr<AnimationTexture>(), new_desc);

		Texture3D::Load_INTERNAL();
	}

	void AnimationTexture::Map()
	{
		Texture3D::Map();

		const GenericTextureDescription& desc = GetDescription();
		PrimitiveTexture* tex = GetPrimitiveTexture();
		TextureMappingTask& map_task = tex->GetMappingTask();

		map_task.Map(
			tex,
			m_evaluated_animations_.data(),
			desc.Width / s_vec4_to_mat,
			desc.Height,
			sizeof(Matrix),
			desc.DepthOrArraySize);
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
