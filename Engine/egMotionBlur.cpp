#include "pch.h"
#include "egMotionBlur.h"

#include "Renderer.h"
#include "egMaterial.h"
#include "egModelRenderer.h"
#include "egSceneManager.hpp"
#include "egTexture2D.h"

namespace Engine::Manager::Graphics
{
  void MotionBlur::Initialize()
  {
    m_previous_depth_.Initialize();
    m_velocity_texture_.Initialize();
    m_previous_depth_.Load();
    m_current_depth_.Load();
    m_velocity_texture_.Load();
    m_motion_blur_shader_ = Resources::Shader::Get("motionblur").lock();
  }

  void MotionBlur::PreUpdate(const float& dt) {}

  void MotionBlur::Update(const float& dt) {}

  void MotionBlur::FixedUpdate(const float& dt) {}

  void MotionBlur::PreRender(const float& dt)
  {
    // Pass #1 : Bind the depth texture to the shader for velocity calculation.
    m_previous_depth_.BindAs(D3D11_BIND_SHADER_RESOURCE, RESERVED_VELOCITY, 0, SHADER_PIXEL);

    ComPtr<ID3D11RenderTargetView> rtv;
    ComPtr<ID3D11DepthStencilView> dsv;

    GetD3Device().GetContext()->OMGetRenderTargets(
        1, rtv.GetAddressOf(), dsv.GetAddressOf());

    GetD3Device().GetContext()->OMSetRenderTargets(
        1, m_velocity_texture_.GetRTVAddress(), m_current_depth_.GetDSV());

    m_motion_blur_shader_->PreRender(dt);
    m_motion_blur_shader_->Render(dt);
    m_previous_depth_.PreRender(dt);
    m_previous_depth_.Render(dt);;

    for (int i = 0; i < SHADER_DOMAIN_MAX; ++i)
    {
      GetRenderer().RenderPass(dt, static_cast<eShaderDomain>(i), true, nullptr);
    }

    m_motion_blur_shader_->PostRender(dt);
    m_previous_depth_.PostRender(dt);

    GetD3Device().GetContext()->OMSetRenderTargets(1, rtv.GetAddressOf(), dsv.Get());
  }

  void MotionBlur::Render(const float& dt)
  {
    // Pass #2 : Bind the velocity texture to the shader for motion blur.
    m_velocity_texture_.BindAs(D3D11_BIND_SHADER_RESOURCE, RESERVED_VELOCITY, 0, SHADER_PIXEL);
    m_velocity_texture_.PreRender(dt);
    m_velocity_texture_.Render(dt);

    if (g_motion_blur_enabled)
    {
      constexpr UINT velocity_slot = 5;
      GetRenderPipeline().SetParam((int)1, velocity_slot);
    }
  }

  void MotionBlur::PostRender(const float& dt)
  {
    // Unbind the velocity texture.
    m_velocity_texture_.PostRender(dt);

    // Clear the velocity texture.
    constexpr float clear_color[4] = { 0.f, 0.f, 0.f, 0.f };
    GetD3Device().GetContext()->ClearRenderTargetView
      (
       m_velocity_texture_.GetRTV(), clear_color
      );
    GetD3Device().GetContext()->ClearDepthStencilView
      (
       m_current_depth_.GetDSV(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0
      );

    // Copy previous frame's depth before clearing rendered scene.
    ComPtr<ID3D11Resource> rp;
    ComPtr<ID3D11Resource> copy;

    GetD3Device().GetDepthStencilView()->GetResource(rp.GetAddressOf());
    m_previous_depth_.GetDSV()->GetResource(copy.GetAddressOf());
    GetD3Device().GetContext()->CopyResource(copy.Get(), rp.Get());
  }

  void MotionBlur::PostUpdate(const float& dt) {}

  void MotionBlur::OnDeserialized()
  {
    Singleton<MotionBlur>::OnDeserialized();
  }
}
