#include "pch.hpp"
#include "egRenderPipeline.hpp"

#include <filesystem>

#include "egD3Device.hpp"
#include "egManagerHelper.hpp"
#include "egToolkitAPI.hpp"

#include "egVertexShaderInternal.hpp"
#include "egShader.hpp"


namespace Engine::Manager::Graphics
{
	void RenderPipeline::SetWorldMatrix(const TransformBuffer& matrix)
	{
		m_transform_buffer_data_.SetData(GetD3Device().GetContext(), matrix);
		GetD3Device().BindConstantBuffer(m_transform_buffer_data_, CB_TYPE_TRANSFORM, SHADER_VERTEX);
	}

	void RenderPipeline::SetWorldMatrix(const TransformBuffer& matrix, const eShaderType shader)
	{
		m_transform_buffer_data_.SetData(GetD3Device().GetContext(), matrix);
		GetD3Device().BindConstantBuffer(m_transform_buffer_data_, CB_TYPE_TRANSFORM, shader);
	}

	void RenderPipeline::SetPerspectiveMatrix(const PerspectiveBuffer& matrix)
	{
		m_wvp_buffer_data_.SetData(GetD3Device().GetContext(), matrix);
		GetD3Device().BindConstantBuffer(m_wvp_buffer_data_, CB_TYPE_WVP, SHADER_VERTEX);
	}

	void RenderPipeline::SetLight(UINT id, const Matrix& world, const Color& color)
	{
		m_light_buffer_.world[id] = world;
		m_light_buffer_.color[id] = color;
	}

	void RenderPipeline::SetShadow(UINT id, const CascadeShadowBuffer& shadow_buffer)
	{
		m_shadow_buffer_data_.SetData(GetD3Device().GetContext(), shadow_buffer);

		GetD3Device().BindConstantBuffer(m_shadow_buffer_data_, CB_TYPE_SHADOW, SHADER_GEOMETRY);
		GetD3Device().BindConstantBuffer(m_shadow_buffer_data_, CB_TYPE_SHADOW, SHADER_PIXEL);
	}

	void RenderPipeline::SetSpecularPower(float power)
	{
		m_specular_buffer_.specular_power = power;
		m_specular_buffer_data_.SetData(GetD3Device().GetContext(), m_specular_buffer_);
		GetD3Device().BindConstantBuffer(m_specular_buffer_data_, CB_TYPE_SPECULAR, SHADER_PIXEL);
	}

	void RenderPipeline::SetSpecularColor(const Color& color)
	{
		m_specular_buffer_.specular_color = color;
		m_specular_buffer_data_.SetData(GetD3Device().GetContext(), m_specular_buffer_);
		GetD3Device().BindConstantBuffer(m_specular_buffer_data_, CB_TYPE_SPECULAR, SHADER_PIXEL);
	}

	void RenderPipeline::BindLightBuffer()
	{
		m_light_buffer_data.SetData(GetD3Device().GetContext(), m_light_buffer_);

		GetD3Device().BindConstantBuffer(m_light_buffer_data, CB_TYPE_LIGHT, SHADER_VERTEX);
		GetD3Device().BindConstantBuffer(m_light_buffer_data, CB_TYPE_LIGHT, SHADER_PIXEL);
		GetD3Device().BindConstantBuffer(m_light_buffer_data, CB_TYPE_LIGHT, SHADER_GEOMETRY);
	}

	void RenderPipeline::SetTopology(const D3D11_PRIMITIVE_TOPOLOGY& topology)
	{
		GetD3Device().GetContext()->IASetPrimitiveTopology(topology);
	}

	void RenderPipeline::ResetRenderTarget()
	{
		GetD3Device().GetContext()->OMSetDepthStencilState(m_depth_stencil_state_.Get(), 0);
		GetD3Device().UpdateRenderTarget();
	}

	void RenderPipeline::SetWireframeState() const
	{
		GetD3Device().GetContext()->RSSetState(GetToolkitAPI().GetCommonStates()->Wireframe());
	}

	void RenderPipeline::SetFillState() const
	{
		GetD3Device().GetContext()->RSSetState(RenderPipeline::m_rasterizer_state_.Get());
	}

	void RenderPipeline::SetNoneCullState() const
	{
		GetD3Device().GetContext()->RSSetState(GetToolkitAPI().GetCommonStates()->CullNone());
	}

	void RenderPipeline::SetFrontCullState() const
	{
		GetD3Device().GetContext()->RSSetState(GetToolkitAPI().GetCommonStates()->CullClockwise());
	}

	void RenderPipeline::BindVertexBuffer(ID3D11Buffer* buffer)
	{
		constexpr UINT stride = sizeof(VertexElement);
		constexpr UINT offset = 0;
		GetD3Device().GetContext()->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
	}

	void RenderPipeline::BindIndexBuffer(ID3D11Buffer* buffer)
	{
		GetD3Device().GetContext()->IASetIndexBuffer(buffer, DXGI_FORMAT_R32_UINT, 0);
	}

	void RenderPipeline::UpdateBuffer(ID3D11Buffer* buffer, const void* data, size_t size)
	{
		GetD3Device().UpdateBuffer(size, data, buffer);
	}

	void RenderPipeline::BindResource(eShaderResource resource, ID3D11ShaderResourceView* texture)
	{
		GetD3Device().GetContext()->PSSetShaderResources(resource, 1, &texture);
	}

	void RenderPipeline::InitializeShadowBuffer()
	{
		ComPtr<ID3D11Texture2D> shadow_map_texture = nullptr;

		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = g_window_width;
		desc.Height = g_window_height;
		desc.MipLevels = 1;
		desc.ArraySize = g_max_shadow_cascades;
		desc.Format = DXGI_FORMAT_R32_TYPELESS;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		GetD3Device().CreateTexture(desc, shadow_map_texture.GetAddressOf());

		D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
		dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
		dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		dsv_desc.Texture2DArray.ArraySize = g_max_shadow_cascades;
		dsv_desc.Texture2DArray.MipSlice = 0;
		dsv_desc.Texture2DArray.FirstArraySlice = 0;
		dsv_desc.Flags = 0;

		DX::ThrowIfFailed(GetD3Device().GetDevice()->CreateDepthStencilView(
					shadow_map_texture.Get(), &dsv_desc, m_shadow_map_depth_view_.GetAddressOf()));

		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
		srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
		srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		srv_desc.Texture2DArray.ArraySize = g_max_shadow_cascades;
		srv_desc.Texture2DArray.FirstArraySlice = 0;
		srv_desc.Texture2DArray.MipLevels = 1;
		srv_desc.Texture2DArray.MostDetailedMip = 0;

		DX::ThrowIfFailed(GetD3Device().GetDevice()->CreateShaderResourceView(
							shadow_map_texture.Get(), &srv_desc, m_shadow_map_resource_view_.GetAddressOf()));

		D3D11_DEPTH_STENCIL_DESC ds_desc{};

		ds_desc.DepthEnable = true;
		ds_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		ds_desc.DepthFunc = D3D11_COMPARISON_LESS;
		ds_desc.StencilEnable = true;
		ds_desc.StencilReadMask = 0xFF;
		ds_desc.StencilWriteMask = 0xFF;
		ds_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		ds_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
		ds_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		ds_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		ds_desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		ds_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
		ds_desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		ds_desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		DX::ThrowIfFailed(GetD3Device().GetDevice()->CreateDepthStencilState(&ds_desc, m_shadow_map_depth_stencil_state_.GetAddressOf()));

		D3D11_SAMPLER_DESC sampler_desc{};
		sampler_desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
		sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
		sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
		sampler_desc.ComparisonFunc = D3D11_COMPARISON_LESS;
		sampler_desc.MinLOD = 0;
		sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
		sampler_desc.MipLODBias = 0.f;
		sampler_desc.MaxAnisotropy = 0;
		sampler_desc.BorderColor[0] = 1.f;
		sampler_desc.BorderColor[1] = 1.f;
		sampler_desc.BorderColor[2] = 1.f;
		sampler_desc.BorderColor[3] = 1.f;

		GetD3Device().CreateSampler(sampler_desc, m_shadow_map_sampler_state_.GetAddressOf());
	}

	void RenderPipeline::Initialize()
	{
		GetD3Device().CreateConstantBuffer(m_wvp_buffer_data_);
		GetD3Device().CreateConstantBuffer(m_transform_buffer_data_);
		GetD3Device().CreateConstantBuffer(m_light_buffer_data);
		GetD3Device().CreateConstantBuffer(m_specular_buffer_data_);
		GetD3Device().CreateConstantBuffer(m_shadow_buffer_data_);

		PrecompileShaders();
		InitializeSamplers();

		InitializeShadowBuffer();

		GetD3Device().CreateBlendState(m_blend_state_.GetAddressOf());
		GetD3Device().CreateRasterizer(m_rasterizer_state_.GetAddressOf(), D3D11_FILL_SOLID);
		GetD3Device().CreateRasterizer(m_rasterizer_state_wire_.GetAddressOf(), D3D11_FILL_WIREFRAME);
		GetD3Device().CreateDepthStencilState(m_depth_stencil_state_.GetAddressOf());

		Engine::GetRenderPipeline().SetSpecularColor({0.5f, 0.5f, 0.5f, 1.0f});
		Engine::GetRenderPipeline().SetSpecularPower(100.0f);
	}

	void RenderPipeline::SetShader(Graphic::IShader* shader)
	{
		switch (shader->GetType())
		{
		case SHADER_VERTEX:
			GetD3Device().BindShader(reinterpret_cast<Graphic::VertexShader*>(shader));
			break;
		case SHADER_PIXEL:
			GetD3Device().BindShader(reinterpret_cast<Graphic::PixelShader*>(shader));
			break;
		case SHADER_GEOMETRY:
			GetD3Device().BindShader(reinterpret_cast<Graphic::GeometryShader*>(shader));
			break;
		case SHADER_COMPUTE:
			GetD3Device().BindShader(reinterpret_cast<Graphic::ComputeShader*>(shader));
			break;
		case SHADER_HULL:
			GetD3Device().BindShader(reinterpret_cast<Graphic::HullShader*>(shader));
			break;
		case SHADER_DOMAIN:
			GetD3Device().BindShader(reinterpret_cast<Graphic::DomainShader*>(shader));
			break;
		default: 
			assert(nullptr);
		}
	}

	void RenderPipeline::PrecompileShaders()
	{
		for (const auto& file : std::filesystem::directory_iterator("./"))
		{
			if (file.path().extension() == ".hlsl")
			{
				const auto prefix = file.path().filename().string();
				const auto filename_without_extension = prefix.substr(0, prefix.find_last_of("."));

				if (prefix.starts_with("vs"))
				{
					boost::shared_ptr<Graphic::Shader<ID3D11VertexShader>> shader = boost::make_shared<Graphic::VertexShader>(filename_without_extension, file);

					shader->Load();
					GetResourceManager().AddResource(filename_without_extension, shader);
				}
				else if (prefix.starts_with("ps"))
				{
					boost::shared_ptr<Graphic::PixelShader> shader = boost::make_shared<Graphic::PixelShader>(
						filename_without_extension, file);

					shader->Load();
					GetResourceManager().AddResource(filename_without_extension, shader);
				}
				else if (prefix.starts_with("gs"))
				{
					boost::shared_ptr<Graphic::GeometryShader> shader = boost::make_shared<Graphic::GeometryShader>(
						filename_without_extension, file);

					shader->Load();
					GetResourceManager().AddResource(filename_without_extension, shader);
				}
				else if (prefix.starts_with("cs"))
				{
					boost::shared_ptr<Graphic::ComputeShader> shader = boost::make_shared<Graphic::ComputeShader>(
						filename_without_extension, file);

					shader->Load();
					GetResourceManager().AddResource(filename_without_extension, shader);
				}
				else if (prefix.starts_with("hs"))
				{
					boost::shared_ptr<Graphic::HullShader> shader = boost::make_shared<Graphic::HullShader>(
						filename_without_extension, file);

					shader->Load();
					GetResourceManager().AddResource(filename_without_extension, shader);
				}
				else if (prefix.starts_with("ds"))
				{
					boost::shared_ptr<Graphic::DomainShader> shader = boost::make_shared<Graphic::DomainShader>(
						filename_without_extension, file);

					shader->Load();
					GetResourceManager().AddResource(filename_without_extension, shader);
				}
			}
		}
	}

	void RenderPipeline::InitializeSamplers()
	{
		const auto sampler = GetToolkitAPI().GetCommonStates()->LinearWrap();

		m_sampler_state_[SAMPLER_TEXTURE] = sampler;
		GetD3Device().BindSampler(m_sampler_state_[SAMPLER_TEXTURE], SHADER_PIXEL, SAMPLER_TEXTURE);
	}

	void RenderPipeline::PreRender(const float& dt)
	{
		// ** overriding DirectXTK common state
		GetD3Device().GetContext()->RSSetState(RenderPipeline::m_rasterizer_state_.Get());
		GetD3Device().GetContext()->OMSetBlendState(RenderPipeline::m_blend_state_.Get(), nullptr, 0xFFFFFFFF);
		GetD3Device().GetContext()->OMSetDepthStencilState(RenderPipeline::m_depth_stencil_state_.Get(), 1);

		GetD3Device().GetContext()->IASetInputLayout(GetRenderPipeline().m_input_layout_.Get());

		ResetShadowMap();
	}

	void RenderPipeline::DrawIndexed(UINT index_count)
	{
		GetD3Device().GetContext()->DrawIndexed(index_count, 0, 0);
	}

	void RenderPipeline::TargetShadowMap()
	{
		ID3D11RenderTargetView* pnullView = nullptr; 

		GetD3Device().GetContext()->OMSetRenderTargets(1, &pnullView, m_shadow_map_depth_view_.Get());
		GetD3Device().GetContext()->OMSetDepthStencilState(m_shadow_map_depth_stencil_state_.Get(), 0);
	}

	void RenderPipeline::BindShadowMap()
	{
		GetD3Device().GetContext()->PSSetShaderResources(SR_SHADOW_MAP, 1, m_shadow_map_resource_view_.GetAddressOf());
	}

	void RenderPipeline::UnbindShadowMap()
	{
		ID3D11ShaderResourceView* pnullView = nullptr;
		GetD3Device().GetContext()->PSSetShaderResources(SR_SHADOW_MAP, 1, &pnullView);
	}

	void RenderPipeline::BindShadowSampler()
	{
		GetD3Device().BindSampler(m_shadow_map_sampler_state_.Get(), SHADER_PIXEL, SAMPLER_SHADOW);
	}

	void RenderPipeline::ResetSampler(const eShaderType shader)
	{
		GetD3Device().BindSampler(m_sampler_state_[SAMPLER_TEXTURE], SHADER_PIXEL, SAMPLER_TEXTURE);
	}

	void RenderPipeline::ResetShaders()
	{
		GetD3Device().GetContext()->VSSetShader(nullptr, nullptr, 0);
		GetD3Device().GetContext()->PSSetShader(nullptr, nullptr, 0);
		GetD3Device().GetContext()->GSSetShader(nullptr, nullptr, 0);
		GetD3Device().GetContext()->CSSetShader(nullptr, nullptr, 0);
		GetD3Device().GetContext()->HSSetShader(nullptr, nullptr, 0);
		GetD3Device().GetContext()->DSSetShader(nullptr, nullptr, 0);
	}

	void RenderPipeline::ResetShadowMap()
	{
		GetD3Device().GetContext()->ClearDepthStencilView(m_shadow_map_depth_view_.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

		GetD3Device().GetContext()->OMSetDepthStencilState(m_depth_stencil_state_.Get(), 0);
	}
}
