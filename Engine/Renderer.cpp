#include "pch.h"
#include "Renderer.h"

#include "egAnimator.h"
#include "egBaseAnimation.h"
#include "egBoneAnimation.h"
#include "egMaterial.h"
#include "egModelRenderer.h"
#include "egProjectionFrustum.h"
#include "egReflectionEvaluator.h"
#include "egSceneManager.hpp"
#include "egShape.h"
#include "egTexture.h"
#include "egTransform.h"

namespace Engine::Manager::Graphics
{
  void Renderer::PreUpdate(const float& dt)
  {
    m_normal_instance_map_.clear();
    m_delayed_instance_map_.clear();
    m_skinned_instance_map_.clear();
    m_animation_texture_map_.clear();
    m_b_ready_ = false;
  }

  void Renderer::Update(const float& dt) {}

  void Renderer::FixedUpdate(const float& dt) {}

  void Renderer::PreRender(const float& dt)
  {
    if (const auto scene = GetSceneManager().GetActiveScene().lock())
    {
      const auto mrs = scene->GetCachedComponents<Components::ModelRenderer>();

      for (const auto& ptr_mr : mrs)
      {
        if (ptr_mr.expired()) { continue; }

        // Retrieve the only object that has the model renderer, if object has the transform information,
        // then add to the instance transformation map. (Object have eventually transform information,
        // if the model renderer is attached to the object, however, it is checked for safe guard.)
        const auto& mr = ptr_mr.lock()->GetSharedPtr<Components::ModelRenderer>();
        const auto& mtr = mr->GetMaterial().lock();
        const auto& obj = mr->GetOwner().lock();
        const auto& tr = obj->GetComponent<Components::Transform>().lock();

        if (!mr->GetActive()) { continue; }
        if (!obj->GetActive()) { continue; }
        if (!tr->GetActive()) { continue; }
        if (!mtr) { continue; }
        if (!tr) { continue; }
        if (!obj) { continue; }

        const bool delayed_flag = (obj->GetObjectType() == DEF_OBJ_T_DELAY_OBJ);
        auto& target = delayed_flag ? m_delayed_instance_map_ : m_normal_instance_map_;

        // Discard the object if it is not in the frustum. Note that delayed objects are not pass-through.
        if (!GetProjectionFrustum().CheckRender(obj)) { continue; }

        // Load the transformation of object. If there is another object that has the same shape and material,
        // then it will be added to the same map and instanced.
        target[mtr][obj] = { tr->GetWorldMatrix().Transpose(), 0 };

        // If there is animation, then prefetch and add to the animation transformation map.
        // There could be different skinned version of the same material.
        if (const auto atr = obj->GetComponent<Components::Animator>().lock())
        {
          target[mtr][obj].bone_flag = 1;

          if (!atr->GetActive())
          {
            target[mtr][obj].bone_flag = 0;
            continue;
          }

          const auto animation_string = atr->GetAnimation();

          if (const auto anim = mtr->GetResource<Resources::BoneAnimation>(animation_string).lock())
          {
            auto transform = anim->GetFrameAnimation(atr->GetFrame());

            if (transform.empty()) { continue; }

            // Transpose for hlsl.
            std::ranges::for_each(transform, [](auto& mat){ mat.Transpose(); });
            m_skinned_instance_map_[mtr][obj] = transform;
          }
        }
      }

      UINT max_bone_count = 0;
      UINT instance_count = 0;

      for (const auto& [mtr, map] : m_skinned_instance_map_)
      {
        for (const auto& bones : map | std::views::values)
        {
          max_bone_count = std::max(max_bone_count, static_cast<UINT>(bones.size()));
          instance_count++;
        }

        D3D11_TEXTURE2D_DESC desc;
        desc.Width              = max_bone_count * sizeof(Matrix);
        desc.Height             = instance_count;
        desc.ArraySize          = 1;
        desc.MipLevels          = -1;
        desc.Format             = DXGI_FORMAT_R32G32B32A32_FLOAT; // one row size
        desc.SampleDesc.Count   = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage              = D3D11_USAGE_DYNAMIC;
        desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags     = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags          = 0;

        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
        srv_desc.Format                    = desc.Format;
        srv_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Texture2D.MostDetailedMip = 0;
        srv_desc.Texture2D.MipLevels       = -1;

        GetD3Device().CreateTexture2D
          (
           desc, srv_desc, m_animation_texture_map_[mtr].texture.ReleaseAndGetAddressOf(),
           m_animation_texture_map_[mtr].srv.ReleaseAndGetAddressOf()
          );

        D3D11_MAPPED_SUBRESOURCE mapped{};
        GetD3Device().GetContext()->Map(
            m_animation_texture_map_[mtr].texture.Get(), 
            0, 
            D3D11_MAP_WRITE_DISCARD, 
            0, 
            &mapped);

        UINT offset = 0;
        for (const auto& bones : map | std::views::values)
        {
          std::memcpy(mapped.pData + offset, bones.data(), sizeof(Matrix) * bones.size());
          offset += sizeof(Matrix) * bones.size();
        }

        GetD3Device().GetContext()->Unmap(m_animation_texture_map_[mtr].texture.Get(), 0);
      }
    }

    m_b_ready_ = true;
  }

  void Renderer::RenderPass(const float& dt, bool post, bool shader)
  {
    if (!m_b_ready_) { throw std::logic_error("Render pass called before instance map initialization"); }

    const auto& target = post ? m_delayed_instance_map_ : m_normal_instance_map_;

    // Create a structured buffer for instance information. (Animation and transformation)
    UINT max_instance_count = 0;

    for (const auto& map : target | std::views::values)
    {
      max_instance_count = std::max(max_instance_count, static_cast<UINT>(map.size()));
    }

    StructuredBuffer<SBs::InstanceSB> instance_sb;
    instance_sb.Create(max_instance_count, nullptr, true);

    // Iterate through the map and render the shape.
    for (const auto& [mtr_ptr, instance_map] : target)
    {
      // Collect the instance information of the current material.
      std::vector<SBs::InstanceSB> instance_data;
      const UINT                   instance_count = static_cast<UINT>(instance_map.size());

      instance_data.reserve(instance_count);

      for (const auto& transform : instance_map | std::views::values) { instance_data.push_back(transform); }

      // Set and bind the instance data to vertex shader.
      instance_sb.SetData(static_cast<UINT>(instance_data.size()), instance_data.data());
      instance_sb.Bind(SHADER_VERTEX);

      // Bind the bone texture.
      if (m_animation_texture_map_.contains(mtr_ptr))
      {
        GetRenderPipeline().BindResource(
          RESERVED_BONES, 
          SHADER_VERTEX, 
          m_animation_texture_map_.at(mtr_ptr).srv.GetAddressOf());
      }

      const auto& mtr = mtr_ptr.lock();

      // This should not be happened.
      if (!mtr) { throw std::logic_error("Material not found unexpectedly"); }

      // Render the material within the instance count.
      mtr->SetInstanceCount(instance_count);
      if (!shader) { mtr->IgnoreShader(); }; // Ignore shader for shadow pass.

      mtr->PreRender(dt);
      mtr->Render(dt);
      mtr->PostRender(dt);

      // Unbind the instance data from vertex shader.
      instance_sb.Unbind(SHADER_VERTEX);
    }

    GetRenderPipeline().UnbindResource(RESERVED_BONES, SHADER_VERTEX);
    instance_sb.Clear();
  }

  void Renderer::Render(const float& dt)
  {
    const auto& scene = GetSceneManager().GetActiveScene().lock();

    RenderPass(dt, false);

    // Notify the reflection evaluator that the rendering is finished.
    GetReflectionEvaluator().RenderFinished();

    RenderPass(dt, true);
  }

  void Renderer::PostRender(const float& dt) {}

  void Renderer::PostUpdate(const float& dt) {}

  void Renderer::Initialize() {}

  bool Renderer::Ready() const { return m_b_ready_; }
}
