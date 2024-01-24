#include "pch.h"
#include "egAnimationsTexture.h"

#include "egBoneAnimation.h"

namespace Engine::Resources
{
  AnimationsTexture::AnimationsTexture(const std::vector<StrongBoneAnimation>& animations)
    : Texture3D("", {}),
      m_animations_(animations) {}

  void AnimationsTexture::PreUpdate(const float& dt) {}

  void AnimationsTexture::Update(const float& dt) {}

  void AnimationsTexture::FixedUpdate(const float& dt) {}

  void AnimationsTexture::PreRender(const float& dt) {}

  void AnimationsTexture::Render(const float& dt)
  {
    BindAs(D3D11_BIND_SHADER_RESOURCE, RESERVED_BONES, 0, SHADER_VERTEX);
    Texture3D::Render(dt);
  }

  void AnimationsTexture::PostRender(const float& dt) { Texture3D::PostRender(dt); }

  void AnimationsTexture::PostUpdate(const float& dt) {}

  void AnimationsTexture::OnDeserialized() { Texture3D::OnDeserialized(); }

  eResourceType AnimationsTexture::GetResourceType() const { return RES_T_ANIMS_TEX; }

  void AnimationsTexture::loadDerived(ComPtr<ID3D11Resource>& res)
  {
    LazyDescription(preEvaluateAnimations(m_animations_, m_evaluated_animations_));

    Texture3D::loadDerived(res);

    Map
      (
       [&](const D3D11_MAPPED_SUBRESOURCE& mapped)
       {
         // Note: Doing dynamic allocation with large data mapping causes a memory race.
         const auto row                  = mapped.RowPitch;
         const auto slice                = mapped.DepthPitch;
         const auto total_texel_in_bytes =
           GetHeight() * mapped.RowPitch +
           GetDepth() * mapped.DepthPitch;

         auto* data = static_cast<float*>(mapped.pData);

         for (int i = 0; i < GetDepth(); ++i)
         {
           UINT d = slice / sizeof(float) * i;

           for (int j = 0; j < GetHeight(); ++j)
           {
             UINT h = row / sizeof(float) * j;
             if (j >= m_evaluated_animations_[i].size()) break;

             for (int k = 0; k < GetWidth() / s_vec4_to_mat; ++k)
             {
               if (k >= m_evaluated_animations_[i][j].size()) break;

               const auto& mat = m_evaluated_animations_[i][j][k];

               std::memcpy(data + d + h + k * s_float_per_mat, &mat, sizeof(Matrix));
             }
           }
         }
       }
      );
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

      for (;t < animation->GetDuration(); t += g_animation_sample_rate)
      {
        const float f_dt  = animation->ConvertDtToFrame(t, animation->GetTicksPerSecond(), animation->GetDuration());
        auto        bones = animation->GetFrameAnimation(f_dt);

        // todo: why transpose is not needed here??
        bone_count = std::max(bone_count, static_cast<UINT>(bones.size()));

        sample_data.push_back(bones);
      }

      preEvaluated.push_back(sample_data);
      frame_count = std::max(frame_count, static_cast<UINT>(sample_data.size()));
    }

    return
    {
        .Width = static_cast<UINT>(bone_count * s_vec4_to_mat),
        .Height = frame_count,
        .Depth = anim_count,
        .ArraySize = 1,
        .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
        .CPUAccessFlags = D3D10_CPU_ACCESS_WRITE,
        .BindFlags = D3D11_BIND_SHADER_RESOURCE,
        .MipsLevel = 1,
        .MiscFlags = 0,
        .Usage = D3D11_USAGE_DYNAMIC,
        .SampleDesc = { .Count = 1, .Quality = 0}
      };
  }
}