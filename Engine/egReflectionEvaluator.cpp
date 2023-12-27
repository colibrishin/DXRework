#include "pch.h"
#include "egReflectionEvaluator.h"

#include "egGlobal.h"

namespace Engine::Manager::Graphics
{
    void ReflectionEvaluator::PreUpdate(const float& dt)
    {
        GetRenderPipeline().UnbindResource(SR_RENDERED, SHADER_PIXEL);
    }

    void ReflectionEvaluator::Update(const float& dt) {}

    void ReflectionEvaluator::FixedUpdate(const float& dt) {}

    void ReflectionEvaluator::PreRender(const float& dt) {}

    void ReflectionEvaluator::Render(const float& dt) {}

    void ReflectionEvaluator::PostRender(const float& dt) {}

    void ReflectionEvaluator::PostUpdate(const float& dt) {}

    void ReflectionEvaluator::Initialize()
    {
        D3D11_TEXTURE2D_DESC desc{};

        desc.Width              = g_window_width;
        desc.Height             = g_window_height;
        desc.MipLevels          = 1;
        desc.ArraySize          = 1;
        desc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count   = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage              = D3D11_USAGE_DEFAULT;
        desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags     = 0;
        desc.MiscFlags          = 0;

        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};

        srv_desc.Format                         = desc.Format;
        srv_desc.ViewDimension                  = D3D11_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Texture2D.MipLevels            = desc.MipLevels;
        srv_desc.Texture2D.MostDetailedMip      = 0;
        srv_desc.Texture2DArray.ArraySize       = 1;
        srv_desc.Texture2DArray.FirstArraySlice = 0;

        GetD3Device().CreateTexture2D(
                                      desc, srv_desc, m_rendered_buffer_.texture.ReleaseAndGetAddressOf(),
                                      m_rendered_buffer_.srv.ReleaseAndGetAddressOf());
    }

    void ReflectionEvaluator::RenderFinished()
    {
        GetD3Device().CopySwapchain(m_rendered_buffer_.srv.Get());
        GetRenderPipeline().BindResource(SR_RENDERED, SHADER_PIXEL, m_rendered_buffer_.srv.GetAddressOf());
    }
}
