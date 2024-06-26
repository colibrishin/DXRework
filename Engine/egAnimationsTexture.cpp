#include "pch.h"
#include "egAnimationsTexture.h"

#include <DirectXTex.h>

#include "egBoneAnimation.h"

SERIALIZE_IMPL
(
 Engine::Resources::AnimationsTexture,
 _ARTAG(_BSTSUPER(Texture3D))
 _ARTAG(m_animations_)
 _ARTAG(m_evaluated_animations_)
)

namespace Engine::Resources
{
  AnimationsTexture::AnimationsTexture(const std::vector<StrongBoneAnimation>& animations)
    : Texture3D("", {}),
      m_animations_(animations) {}

  void AnimationsTexture::PreUpdate(const float& dt) {}

  void AnimationsTexture::Update(const float& dt) {}

  void AnimationsTexture::FixedUpdate(const float& dt) {}

  void AnimationsTexture::PreRender(const float& dt)
  {
    Texture3D::PreRender(dt);
  }

  void AnimationsTexture::Render(const float& dt)
  {
    Texture3D::Render(dt);
  }

  void AnimationsTexture::PostRender(const float& dt) { Texture3D::PostRender(dt); }

  void AnimationsTexture::PostUpdate(const float& dt) {}

  void AnimationsTexture::OnSerialized()
  {
    Texture3D::OnSerialized();

    for (const auto& anim : m_animations_)
    {
      Serializer::Serialize(anim->GetName(), anim);
    }
  }

  void AnimationsTexture::OnDeserialized() { Texture3D::OnDeserialized(); }

  eResourceType AnimationsTexture::GetResourceType() const { return RES_T_ANIMS_TEX; }

  void AnimationsTexture::loadDerived(ComPtr<ID3D12Resource>& res)
  {
    LazyDescription(preEvaluateAnimations(m_animations_, m_evaluated_animations_));

    Texture3D::loadDerived(res);
  }

  bool AnimationsTexture::map(char* mapped)
  {
    Texture3D::map(mapped);

    // Note: Doing dynamic allocation with large data mapping causes a memory race.
    const D3D12_RESOURCE_DESC          desc            = GetRawResoruce()->GetDesc();
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT place_footprint = {};

    size_t row_pitch;
    size_t slice_pitch;

    DX::ThrowIfFailed(DirectX::ComputePitch(DXGI_FORMAT_R32G32B32A32_FLOAT, desc.Width, desc.Height, row_pitch, slice_pitch));

    auto* data = reinterpret_cast<float*>(mapped);

    for (UINT i = 0; i < GetDepth(); ++i)
    {
      const UINT d = slice_pitch / sizeof(float) * i;

      for (UINT j = 0; j < GetHeight(); ++j)
      {
        const UINT h = row_pitch / sizeof(float) * j;
        if (j >= m_evaluated_animations_[i].size()) break;

        for (UINT k = 0; k < GetWidth() / s_vec4_to_mat; ++k)
        {
          if (k >= m_evaluated_animations_[i][j].size()) break;

          const auto& mat = m_evaluated_animations_[i][j][k];

          _mm256_memcpy(data + d + h + k * s_float_per_mat, &mat, sizeof(Matrix));
        }
      }
    }

    return true;
  }

  Texture::GenericTextureDescription AnimationsTexture::preEvaluateAnimations(const std::vector<StrongBoneAnimation>& anims, std::vector<std::vector<std::vector<Matrix>>>& preEvaluated)
  {
    const UINT anim_count = static_cast<UINT>(anims.size());
    UINT frame_count = 0;
    UINT bone_count = 0;

    for (const auto& animation : anims)
    {
      std::vector<std::vector<Matrix>> sample_data;
      float t = 0.f;

      for (;t < animation->GetDuration() / animation->GetTicksPerSecond(); 
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
      .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
      .Flags = D3D12_RESOURCE_FLAG_NONE,
      .MipsLevel = 1,
      .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
      .SampleDesc = { .Count = 1, .Quality = 0}
      };
  }
}
