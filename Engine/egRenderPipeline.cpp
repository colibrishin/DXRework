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

	void RenderPipeline::SetPerspectiveMatrix(const PerspectiveBuffer& matrix)
	{
		m_wvp_buffer_data_.SetData(GetD3Device().GetContext(), matrix);
		GetD3Device().BindConstantBuffer(m_wvp_buffer_data_, CB_TYPE_WVP, SHADER_VERTEX);
	}

	void RenderPipeline::SetLightPosition(UINT id, const Vector3& position)
	{
		m_light_position_buffer_.position[id] = Vector4(position.x, position.y, position.z, 1.0f);
	}

	void RenderPipeline::SetLightColor(UINT id, const Vector4& color)
	{
		m_light_color_buffer_.color[id] = color;
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

	void RenderPipeline::BindLightBuffers()
	{
		m_light_position_buffer_data_.SetData(GetD3Device().GetContext(), m_light_position_buffer_);
		GetD3Device().BindConstantBuffer(m_light_position_buffer_data_, CB_TYPE_LIGHT_POSITION, SHADER_VERTEX);

		m_light_color_buffer_data_.SetData(GetD3Device().GetContext(), m_light_color_buffer_);
		GetD3Device().BindConstantBuffer(m_light_color_buffer_data_, CB_TYPE_LIGHT_COLOR, SHADER_PIXEL);
	}

	void RenderPipeline::SetTopology(const D3D11_PRIMITIVE_TOPOLOGY& topology)
	{
		GetD3Device().GetContext()->IASetPrimitiveTopology(topology);
	}

	void RenderPipeline::SetWireframeState() const
	{
		GetD3Device().GetContext()->RSSetState(RenderPipeline::m_rasterizer_state_wire_.Get());
	}

	void RenderPipeline::SetFillState() const
	{
		GetD3Device().GetContext()->RSSetState(RenderPipeline::m_rasterizer_state_.Get());
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

	void RenderPipeline::Initialize()
	{
		GetD3Device().CreateConstantBuffer(m_wvp_buffer_data_);
		GetD3Device().CreateConstantBuffer(m_transform_buffer_data_);
		GetD3Device().CreateConstantBuffer(m_light_position_buffer_data_);
		GetD3Device().CreateConstantBuffer(m_light_color_buffer_data_);
		GetD3Device().CreateConstantBuffer(m_specular_buffer_data_);

		PrecompileShaders();
		InitializeSamplers();

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

		s_sampler_state_[SHADER_VERTEX] = sampler;
		GetD3Device().BindSampler(s_sampler_state_[SHADER_VERTEX], SHADER_VERTEX);

		s_sampler_state_[SHADER_PIXEL] = sampler;
		GetD3Device().BindSampler(s_sampler_state_[SHADER_PIXEL], SHADER_PIXEL);

		s_sampler_state_[SHADER_GEOMETRY] = sampler;
		GetD3Device().BindSampler(s_sampler_state_[SHADER_GEOMETRY], SHADER_GEOMETRY);

		s_sampler_state_[SHADER_COMPUTE] = sampler;
		GetD3Device().BindSampler(s_sampler_state_[SHADER_COMPUTE], SHADER_COMPUTE);

		s_sampler_state_[SHADER_HULL] = sampler;
		GetD3Device().BindSampler(s_sampler_state_[SHADER_HULL], SHADER_HULL);

		s_sampler_state_[SHADER_DOMAIN] = sampler;
		GetD3Device().BindSampler(s_sampler_state_[SHADER_DOMAIN], SHADER_DOMAIN);
	}

	void RenderPipeline::PreRender(const float& dt)
	{
		// ** overriding DirectXTK common state
		GetD3Device().GetContext()->RSSetState(RenderPipeline::m_rasterizer_state_.Get());
		GetD3Device().GetContext()->OMSetBlendState(RenderPipeline::m_blend_state_.Get(), nullptr, 0xFFFFFFFF);
		GetD3Device().GetContext()->OMSetDepthStencilState(RenderPipeline::m_depth_stencil_state_.Get(), 1);

		GetD3Device().GetContext()->IASetInputLayout(GetRenderPipeline().m_input_layout_.Get());
	}

	void RenderPipeline::DrawIndexed(UINT index_count)
	{
		GetD3Device().GetContext()->DrawIndexed(index_count, 0, 0);
	}
}
