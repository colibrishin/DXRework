#pragma once
#include "pch.hpp"
#include "egRenderPipeline.hpp"

#include <filesystem>

#include "egD3Device.hpp"
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
		D3Device::BindConstantBuffer(s_vp_buffer_data_, CB_TYPE_TRANSFORM, SHADER_VERTEX);
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

	void RenderPipeline::Initialize()
	{
		if (s_instance_)
		{
			return;
		}

		s_instance_ = std::unique_ptr<RenderPipeline>(new RenderPipeline);

		D3Device::CreateConstantBuffer(s_vp_buffer_data_);
		D3Device::CreateConstantBuffer(s_transform_buffer_data_);

		CompileShaders();

		D3Device::CreateBlendState(s_blend_state_.GetAddressOf());
		D3Device::CreateRasterizer(s_rasterizer_state_.GetAddressOf());
	}

	void RenderPipeline::SetShader(const std::wstring& name)
	{
		if(!s_shader_map_.contains(name))
		{
			return;
		}

		IShader* shader = s_shader_map_.at(name).get();

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

	void RenderPipeline::CompileShaders()
	{
		for (const auto& file : std::filesystem::directory_iterator("./"))
		{
			if(file.path().extension() == L".hlsl")
			{
				const auto prefix = file.path().filename().wstring();
				const auto filename_without_extension = prefix.substr(0, prefix.find_last_of(L"."));

				if (prefix.starts_with(L"vs"))
				{
					s_shader_map_.emplace(filename_without_extension, std::make_shared<VertexShader>(filename_without_extension, file.path()));
					std::shared_ptr<VertexShader> shader = std::reinterpret_pointer_cast<VertexShader>(s_shader_map_[filename_without_extension]);

					D3Device::CreateShader(std::filesystem::absolute(file), shader.get());
				}
				else if (prefix.starts_with(L"ps"))
				{
					s_shader_map_.emplace(filename_without_extension, std::make_shared<Shader<ID3D11PixelShader>>(filename_without_extension, file.path()));
					std::shared_ptr<Shader<ID3D11PixelShader>> shader = std::reinterpret_pointer_cast<Shader<ID3D11PixelShader>>(s_shader_map_[filename_without_extension]);

					D3Device::CreateShader(std::filesystem::absolute(file), shader.get());
				}
				else if (prefix.starts_with(L"gs"))
				{
					s_shader_map_.emplace(filename_without_extension, std::make_shared<Shader<ID3D11GeometryShader>>(filename_without_extension, file.path()));
					std::shared_ptr<Shader<ID3D11GeometryShader>> shader = std::reinterpret_pointer_cast<Shader<ID3D11GeometryShader>>(s_shader_map_[filename_without_extension]);

					D3Device::CreateShader(std::filesystem::absolute(file), shader.get());
				}
				else if (prefix.starts_with(L"cs"))
				{
					s_shader_map_.emplace(filename_without_extension, std::make_shared<Shader<ID3D11ComputeShader>>(filename_without_extension, file.path()));
					std::shared_ptr<Shader<ID3D11ComputeShader>> shader = std::reinterpret_pointer_cast<Shader<ID3D11ComputeShader>>(s_shader_map_[filename_without_extension]);

					D3Device::CreateShader(std::filesystem::absolute(file), shader.get());
				}
				else if (prefix.starts_with(L"hs"))
				{
					s_shader_map_.emplace(filename_without_extension, std::make_shared<Shader<ID3D11HullShader>>(filename_without_extension, file.path()));
					std::shared_ptr<Shader<ID3D11HullShader>> shader = std::reinterpret_pointer_cast<Shader<ID3D11HullShader>>(s_shader_map_[filename_without_extension]);

					D3Device::CreateShader(std::filesystem::absolute(file), shader.get());
				}
				else if (prefix.starts_with(L"ds"))
				{
					s_shader_map_.emplace(filename_without_extension, std::make_shared<Shader<ID3D11DomainShader>>(filename_without_extension, file.path()));
					std::shared_ptr<Shader<ID3D11DomainShader>> shader = std::reinterpret_pointer_cast<Shader<ID3D11DomainShader>>(s_shader_map_[filename_without_extension]);

					D3Device::CreateShader(std::filesystem::absolute(file), shader.get());
				}
			}
		}
	}

	void RenderPipeline::DrawIndexed(UINT index_count)
	{
		D3Device::s_context_->DrawIndexed(index_count, 0, 0);
	}
}
