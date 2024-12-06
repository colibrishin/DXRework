#include "../Public/AnimationTexture.h"

#include <DirectXTex.h>
#include <algorithm>

#include "Source/Runtime/Resources/BoneAnimation/Public/BoneAnimation.h"
#include "Source/Runtime/Core/SIMDExtension/Public/SIMDExtension.hpp"

SERIALIZE_IMPL
(
 Engine::Resources::AnimationTexture,
 _ARTAG(_BSTSUPER(Texture3D))
 _ARTAG(m_animations_)
 _ARTAG(m_evaluated_animations_)
)

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

		for (const auto& anim : m_animations_)
		{
			Serializer::Serialize(anim->GetName(), anim);
		}
	}

	void AnimationTexture::OnDeserialized()
	{
		Texture3D::OnDeserialized();
	}

	eResourceType AnimationTexture::GetResourceType() const
	{
		return RES_T_ANIMS_TEX;
	}

	void AnimationTexture::loadDerived(ComPtr<ID3D12Resource>& res)
	{
		LazyDescription(preEvaluateAnimations(m_animations_, m_evaluated_animations_));

		Texture3D::loadDerived(res);
	}

	bool AnimationTexture::map(char* mapped)
	{
		Texture3D::map(mapped);

		const D3D12_RESOURCE_DESC          desc            = GetRawResoruce()->GetDesc();

		// Align(Width * format bytes, 256) = Row pitch
		size_t row_pitch = Align(GetWidth() * (DirectX::BitsPerPixel(desc.Format) / 8), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
		// RowPitch * Height = Slice pitch
		size_t slice_pitch = Align(GetHeight() * row_pitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

		auto* data = reinterpret_cast<float*>(mapped);

		for (UINT64 i = 0; i < GetDepth(); ++i)
		{
			const UINT64 d = slice_pitch / sizeof(float) * i;

			for (UINT64 j = 0; j < GetHeight(); ++j)
			{
				const UINT64 h = row_pitch / sizeof(float) * j;
				if (j >= m_evaluated_animations_[i].size())
				{
					break;
				}

				for (UINT64 k = 0; k < GetWidth() / s_vec4_to_mat; ++k)
				{
					if (k >= m_evaluated_animations_[i][j].size())
					{
						break;
					}

					const auto& mat = m_evaluated_animations_[i][j][k];

					SIMDExtension::_mm256_memcpy(data + d + h + k * s_float_per_mat, &mat, sizeof(Matrix));
				}
			}
		}

		return true;
	}

	Texture::GenericTextureDescription AnimationTexture::preEvaluateAnimations(
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
			.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D,
			.Alignment = 0,
			.Width = static_cast<UINT>(bone_count * s_vec4_to_mat),
			.Height = frame_count,
			.DepthOrArraySize = static_cast<UINT16>(anim_count),
			.Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
			.Flags = D3D12_RESOURCE_FLAG_NONE,
			.MipsLevel = 1,
			.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
			.SampleDesc = {.Count = 1, .Quality = 0}
		};
	}
}
