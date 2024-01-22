#include "pch.h"
#include "egAnimations.h"

#include "egBoneAnimation.h"

namespace Engine::Resources
{
  Animations::Animations(const std::vector<StrongBoneAnimation>& animations)
    : Resource("", RES_T_ANIMS),
      m_animations_(animations),
      m_animation_count_(0),
      m_max_bone_count_(0),
      m_max_frame_count_(0) {}

  void Animations::PreUpdate(const float& dt) {}

  void Animations::Update(const float& dt) {}

  void Animations::FixedUpdate(const float& dt) {}

  void Animations::PreRender(const float& dt) {}

  void Animations::Render(const float& dt)
  {
    GetRenderPipeline().BindResource(RESERVED_BONES, SHADER_VERTEX, m_srv_.GetAddressOf());
  }

  void Animations::PostRender(const float& dt)
  {
    GetRenderPipeline().UnbindResource(RESERVED_BONES, SHADER_VERTEX);
  }

  void Animations::PostUpdate(const float& dt) {}

  void Animations::OnDeserialized() { Resource::OnDeserialized(); }

  eResourceType Animations::GetResourceType() const { return Resource::GetResourceType(); }

  void Animations::Load_INTERNAL()
  {
    std::vector<std::vector<std::vector<Matrix>>> animations;
    m_animation_count_ = static_cast<UINT>(m_animations_.size());

    for (const auto& animation : m_animations_)
    {
      std::vector<std::vector<Matrix>> sample_data;
      float t = 0.f;

      for (;t < animation->GetDuration(); t += g_animation_sample_rate)
      {
        const float f_dt  = animation->ConvertDtToFrame(t, animation->GetTicksPerSecond(), animation->GetDuration());
        auto        bones = animation->GetFrameAnimation(f_dt);

        std::ranges::for_each(bones, [](Matrix& mat) { mat = mat.Transpose(); });
        m_max_bone_count_ = std::max(m_max_bone_count_, static_cast<UINT>(bones.size()));

        sample_data.push_back(bones);
      }

      animations.push_back(sample_data);
      m_max_frame_count_ = std::max(m_max_frame_count_, static_cast<UINT>(sample_data.size()));
    }

    const D3D11_TEXTURE3D_DESC dsc
    {
      .Width = static_cast<UINT>(m_max_bone_count_ * 4), // 16 bytes, 4 floats for matrix
      .Height = m_max_frame_count_,
      .Depth = m_animation_count_,
      .MipLevels = 1,
      .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
      .Usage = D3D11_USAGE_DYNAMIC,
      .BindFlags = D3D11_BIND_SHADER_RESOURCE,
      .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
      .MiscFlags = 0
    };

    constexpr D3D11_SHADER_RESOURCE_VIEW_DESC srv_dsc
    {
      .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
      .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D,
      .Texture3D =
      {
        .MostDetailedMip = 0,
        .MipLevels = 1
      },
    };

    GetD3Device().GetDevice()->CreateTexture3D(&dsc, nullptr, &m_tex_);

    D3D11_MAPPED_SUBRESOURCE mapped;

    GetD3Device().GetContext()->Map(m_tex_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

    // Note: Doing dynamic allocation with large data mapping causes a memory race.
    const auto row       = mapped.RowPitch;
    const auto slice      = mapped.DepthPitch;
    const auto total_texel_in_bytes = 
        m_max_frame_count_ * mapped.RowPitch + 
        m_animation_count_ * mapped.DepthPitch;

    constexpr auto matrix_float_count = sizeof(Matrix) / sizeof(float);

    auto* data = static_cast<float*>(mapped.pData);

    for (int i = 0; i < m_animation_count_; ++i)
    {
      UINT d = slice / sizeof(float) * i;

      for (int j = 0; j < m_max_frame_count_; ++j)
      {
        UINT h = row  / sizeof(float) * j;
        if (j >= animations[i].size()) break;

        for (int k = 0; k < m_max_bone_count_; ++k)
        {
          if (k >= animations[i][j].size()) break;

          const auto& mat = animations[i][j][k];

          std::memcpy(data + d + h + k * matrix_float_count, &mat, sizeof(Matrix));
        }
      }
    }

    GetD3Device().GetContext()->Unmap(m_tex_.Get(), 0);

    GetD3Device().GetDevice()->CreateShaderResourceView(m_tex_.Get(), &srv_dsc, &m_srv_);
  }

  void Animations::Unload_INTERNAL()
  {
    m_tex_.Reset();
    m_srv_.Reset();
  }
}
