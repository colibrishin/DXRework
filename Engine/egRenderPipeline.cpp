#include "pch.hpp"
#include "egRenderPipeline.hpp"

#include <filesystem>

#include "egD3Device.hpp"
#include "egManagerHelper.hpp"
#include "egToolkitAPI.hpp"
#include "egVertexShader.hpp"

namespace Engine::Graphic
{
	void RenderPipeline::SetWorldMatrix(const TransformBuffer& matrix)
	{
		s_transform_buffer_data_.SetData(D3Device::s_context_.Get(), matrix);
		D3Device::BindConstantBuffer(s_transform_buffer_data_, CB_TYPE_TRANSFORM, SHADER_VERTEX);
	}

	void RenderPipeline::SetPerspectiveMatrix(const VPBuffer& matrix)
	{
		s_vp_buffer_data_.SetData(D3Device::s_context_.Get(), matrix);
		D3Device::BindConstantBuffer(s_vp_buffer_data_, CB_TYPE_VP, SHADER_VERTEX);
	}

	void RenderPipeline::SetLightPosition(UINT id, const Vector3& position)
	{
		s_light_position_buffer_.position[id] = Vector4(position.x, position.y, position.z, 1.0f);
	}

	void RenderPipeline::SetLightColor(UINT id, const Vector4& color)
	{
		s_light_color_buffer_.color[id] = color;
	}

	void RenderPipeline::BindLightBuffers()
	{
		s_light_position_buffer_data_.SetData(D3Device::s_context_.Get(), s_light_position_buffer_);
		D3Device::BindConstantBuffer(s_light_position_buffer_data_, CB_TYPE_LIGHT_POSITION, SHADER_VERTEX);

		s_light_color_buffer_data_.SetData(D3Device::s_context_.Get(), s_light_color_buffer_);
		D3Device::BindConstantBuffer(s_light_color_buffer_data_, CB_TYPE_LIGHT_COLOR, SHADER_PIXEL);
	}

	void RenderPipeline::SetTopology(const D3D11_PRIMITIVE_TOPOLOGY& topology)
	{
		D3Device::s_context_->IASetPrimitiveTopology(topology);
	}

	void RenderPipeline::BindVertexBuffer(ID3D11Buffer* buffer)
	{
		constexpr UINT stride = sizeof(VertexElement);
		constexpr UINT offset = 0;
		D3Device::s_context_->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
	}

	void RenderPipeline::BindIndexBuffer(ID3D11Buffer* buffer)
	{
		D3Device::s_context_->IASetIndexBuffer(buffer, DXGI_FORMAT_R32_UINT, 0);
	}

	void RenderPipeline::UpdateBuffer(ID3D11Buffer* buffer, const void* data, size_t size)
	{
		D3Device::UpdateBuffer(size, data, buffer);
	}

	void RenderPipeline::BindTexture(ID3D11ShaderResourceView* texture)
	{
		D3Device::s_context_->PSSetShaderResources(SR_TEXTURE, 1, &texture);
	}

	void RenderPipeline::Initialize()
	{
		if (s_instance_)
		{
			return;
		}

		s_instance_ = std::unique_ptr<RenderPipeline>(new RenderPipeline);

		D3Device::CreateConstantBuffer(s_vp_buffer_data_);
		D3Device::CreateConstantBuffer(s_transform_buffer_data_);
		D3Device::CreateConstantBuffer(s_light_position_buffer_data_);
		D3Device::CreateConstantBuffer(s_light_color_buffer_data_);

		PrecompileShaders();
		InitializeSamplers();

		D3Device::CreateBlendState(s_blend_state_.GetAddressOf());
		D3Device::CreateRasterizer(s_rasterizer_state_.GetAddressOf());
		D3Device::CreateDepthStencilState(s_depth_stencil_state_.GetAddressOf());
	}

	void RenderPipeline::SetShader(IShader* shader)
	{
		switch (shader->GetType())
		{
		case SHADER_VERTEX:
			D3Device::BindShader(reinterpret_cast<VertexShader*>(shader));
			break;
		case SHADER_PIXEL:
			D3Device::BindShader(reinterpret_cast<Shader<ID3D11PixelShader>*>(shader));
			break;
		case SHADER_GEOMETRY:
			D3Device::BindShader(reinterpret_cast<Shader<ID3D11GeometryShader>*>(shader));
			break;
		case SHADER_COMPUTE:
			D3Device::BindShader(reinterpret_cast<Shader<ID3D11ComputeShader>*>(shader));
			break;
		case SHADER_HULL:
			D3Device::BindShader(reinterpret_cast<Shader<ID3D11HullShader>*>(shader));
			break;
		case SHADER_DOMAIN:
			D3Device::BindShader(reinterpret_cast<Shader<ID3D11DomainShader>*>(shader));
			break;
		}
	}

	void RenderPipeline::PrecompileShaders()
	{
		for (const auto& file : std::filesystem::directory_iterator("./"))
		{
			if (file.path().extension() == L".hlsl")
			{
				const auto prefix = file.path().filename().wstring();
				const auto filename_without_extension = prefix.substr(0, prefix.find_last_of(L"."));

				if (prefix.starts_with(L"vs"))
				{
					std::shared_ptr<Shader<ID3D11VertexShader>> shader = std::make_shared<VertexShader>(filename_without_extension, file);

					D3Device::CreateShader(absolute(file), shader.get());
					GetResourceManager()->AddResource<VertexShader>(filename_without_extension, std::reinterpret_pointer_cast<VertexShader>(shader));
				}
				else if (prefix.starts_with(L"ps"))
				{
					std::shared_ptr<Shader<ID3D11PixelShader>> shader = std::make_shared<Shader<ID3D11PixelShader>>(
						filename_without_extension, file);

					D3Device::CreateShader(absolute(file), shader.get());
					GetResourceManager()->AddResource(filename_without_extension, shader);
				}
				else if (prefix.starts_with(L"gs"))
				{
					std::shared_ptr<Shader<ID3D11GeometryShader>> shader = std::make_shared<Shader<ID3D11GeometryShader>>(
						filename_without_extension, file);

					D3Device::CreateShader(absolute(file), shader.get());
					GetResourceManager()->AddResource(filename_without_extension, shader);
				}
				else if (prefix.starts_with(L"cs"))
				{
					std::shared_ptr<Shader<ID3D11ComputeShader>> shader = std::make_shared<Shader<ID3D11ComputeShader>>(
						filename_without_extension, file);

					D3Device::CreateShader(absolute(file), shader.get());
					GetResourceManager()->AddResource(filename_without_extension, shader);
				}
				else if (prefix.starts_with(L"hs"))
				{
					std::shared_ptr<Shader<ID3D11HullShader>> shader = std::make_shared<Shader<ID3D11HullShader>>(
						filename_without_extension, file);

					D3Device::CreateShader(absolute(file), shader.get());
					GetResourceManager()->AddResource(filename_without_extension, shader);
				}
				else if (prefix.starts_with(L"ds"))
				{
					std::shared_ptr<Shader<ID3D11DomainShader>> shader = std::make_shared<Shader<ID3D11DomainShader>>(
						filename_without_extension, file);

					D3Device::CreateShader(absolute(file), shader.get());
					GetResourceManager()->AddResource(filename_without_extension, shader);
				}
			}
		}
	}

	void RenderPipeline::InitializeSamplers()
	{
		s_sampler_state_[SHADER_VERTEX] = ToolkitAPI::m_states_->LinearWrap();
		D3Device::BindSampler(s_sampler_state_[SHADER_VERTEX], SHADER_VERTEX);

		s_sampler_state_[SHADER_PIXEL] = ToolkitAPI::m_states_->LinearWrap();
		D3Device::BindSampler(s_sampler_state_[SHADER_PIXEL], SHADER_PIXEL);

		s_sampler_state_[SHADER_GEOMETRY] = ToolkitAPI::m_states_->LinearWrap();
		D3Device::BindSampler(s_sampler_state_[SHADER_GEOMETRY], SHADER_GEOMETRY);

		s_sampler_state_[SHADER_COMPUTE] = ToolkitAPI::m_states_->LinearWrap();
		D3Device::BindSampler(s_sampler_state_[SHADER_COMPUTE], SHADER_COMPUTE);

		s_sampler_state_[SHADER_HULL] = ToolkitAPI::m_states_->LinearWrap();
		D3Device::BindSampler(s_sampler_state_[SHADER_HULL], SHADER_HULL);

		s_sampler_state_[SHADER_DOMAIN] = ToolkitAPI::m_states_->LinearWrap();
		D3Device::BindSampler(s_sampler_state_[SHADER_DOMAIN], SHADER_DOMAIN);
	}

	void RenderPipeline::DrawIndexed(UINT index_count)
	{
		D3Device::s_context_->DrawIndexed(index_count, 0, 0);
	}
}
