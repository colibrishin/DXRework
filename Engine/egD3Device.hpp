#pragma once
#include "pch.hpp"

#include <filesystem>

#include "egApplication.hpp"
#include "egRenderPipeline.hpp"
#include "egVertexShader.hpp"

namespace Engine::Graphic
{
	using namespace DirectX;

	class D3Device final
	{
	public:
		~D3Device() = default;
		D3Device(const D3Device&) = delete;

		static void Initialize(HWND hwnd);

		static float GetAspectRatio()
		{
			return static_cast<float>(g_window_width) / static_cast<float>(g_window_height);
		}

		static void UpdateRenderTarget();
		static void UpdateViewport();

		template <typename T>
		static void CreateBuffer(D3D11_BIND_FLAG flag, UINT size, ID3D11Buffer** buffer)
		{
			D3D11_BUFFER_DESC desc{};

			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.BindFlags = flag;
			desc.ByteWidth = size * sizeof(T);
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;

			DX::ThrowIfFailed(s_device_->CreateBuffer(&desc, nullptr, buffer));
		}

		template <typename T>
		static void CreateBuffer(D3D11_BIND_FLAG flag, UINT size, ID3D11Buffer** buffer, void* initial_data)
		{
			D3D11_BUFFER_DESC desc{};

			desc.Usage = D3D11_USAGE_IMMUTABLE;
			desc.CPUAccessFlags = 0;
			desc.BindFlags = flag;
			desc.ByteWidth = size * sizeof(T);
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;

			D3D11_SUBRESOURCE_DATA data{};
			data.pSysMem = initial_data;

			DX::ThrowIfFailed(s_device_->CreateBuffer(&desc, &data, buffer));
		}

		static void CreateTextureFromFile(const std::filesystem::path& path, ID3D11Resource** texture,
		                                  ID3D11ShaderResourceView** shader_resource_view);

		template <typename T>
		static void CreateShader(const std::filesystem::path& path, Shader<T>* shader)
		{
			ComPtr<ID3DBlob> blob;
			ComPtr<ID3DBlob> error;
			const eShaderType type = g_shader_enum_type_map.at(__uuidof(T));

			UINT flag = D3DCOMPILE_ENABLE_STRICTNESS;

#if defined(_DEBUG)
			flag |= D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG;
#endif

			DX::ThrowIfFailed(
				D3DCompileFromFile(
					path.c_str(),
					nullptr,
					nullptr,
					"main",
					g_shader_target_map.at(type).c_str(),
					flag,
					0,
					&blob,
					&error));

			if constexpr (std::is_same_v<T, ID3D11VertexShader>)
			{
				const auto input_descs = GenerateInputDescription(shader, blob.Get());
				const auto casted = dynamic_cast<VertexShader*>(shader);

				DX::ThrowIfFailed(s_device_->CreateInputLayout(input_descs.data(), input_descs.size(),
				                                               blob->GetBufferPointer(), blob->GetBufferSize(),
				                                               casted->GetInputLayout()));
				DX::ThrowIfFailed(s_device_->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(),
				                                                nullptr, shader->GetShader()));
			}
			else if constexpr (std::is_same_v<T, ID3D11PixelShader>)
			{
				DX::ThrowIfFailed(s_device_->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr,
				                                               shader->GetShader()));
			}
			else if constexpr (std::is_same_v<T, ID3D11GeometryShader>)
			{
				DX::ThrowIfFailed(s_device_->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(),
				                                                  nullptr, shader->GetShader()));
			}
			else if constexpr (std::is_same_v<T, ID3D11ComputeShader>)
			{
				DX::ThrowIfFailed(s_device_->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(),
				                                                 nullptr, shader->GetShader()));
			}
			else if constexpr (std::is_same_v<T, ID3D11HullShader>)
			{
				DX::ThrowIfFailed(s_device_->CreateHullShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr,
				                                              shader->GetShader()));
			}
			else if constexpr (std::is_same_v<T, ID3D11DomainShader>)
			{
				DX::ThrowIfFailed(s_device_->CreateDomainShader(blob->GetBufferPointer(), blob->GetBufferSize(),
				                                                nullptr, shader->GetShader()));
			}
		}

		static void FrameBegin();
		static void Present();

		static Matrix GetWorldMatrix() { return s_world_matrix_; }
		static Matrix GetProjectionMatrix() { return s_projection_matrix_; }
		static ID3D11Device* GetDevice() { return s_device_.Get(); }

	private:
		friend class RenderPipeline;
		friend class ToolkitAPI;

		D3Device() = default;

		static void UpdateBuffer(UINT size, const void* data, ID3D11Buffer* buffer)
		{
			D3D11_MAPPED_SUBRESOURCE mapped_resource{};

			DX::ThrowIfFailed(s_context_->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource));
			memcpy(mapped_resource.pData, data, size);
			s_context_->Unmap(buffer, 0);
		}

		template <typename T>
		static void CreateConstantBuffer(ConstantBuffer<T>& buffer)
		{
			buffer.Create(s_device_.Get());
		}

		template <typename T>
		static void BindConstantBuffer(ConstantBuffer<T>& buffer, eCBType type, eShaderType target_shader)
		{
			g_shader_cb_bind_map.at(target_shader)(s_device_.Get(), s_context_.Get(), buffer.GetBuffer(), type, 1);
		}

		static std::vector<D3D11_INPUT_ELEMENT_DESC> GenerateInputDescription(
			Shader<ID3D11VertexShader>* shader, ID3DBlob* blob);

		template <typename T>
		static void BindShader(Shader<T>* shader)
		{
			if constexpr (std::is_same_v<T, ID3D11VertexShader>)
			{
				auto casting = static_cast<VertexShader*>(shader);
				s_context_->VSSetShader(*(casting->GetShader()), nullptr, 0);
				s_context_->IASetInputLayout(*casting->GetInputLayout());
			}
			else if constexpr (std::is_same_v<T, ID3D11PixelShader>)
			{
				s_context_->PSSetShader(*(shader->GetShader()), nullptr, 0);
			}
			else if constexpr (std::is_same_v<T, ID3D11GeometryShader>)
			{
				s_context_->GSSetShader(*(shader->GetShader()), nullptr, 0);
			}
			else if constexpr (std::is_same_v<T, ID3D11ComputeShader>)
			{
				s_context_->CSSetShader(*(shader->GetShader()), nullptr, 0);
			}
			else if constexpr (std::is_same_v<T, ID3D11HullShader>)
			{
				s_context_->HSSetShader(*(shader->GetShader()), nullptr, 0);
			}
			else if constexpr (std::is_same_v<T, ID3D11DomainShader>)
			{
				s_context_->DSSetShader(*(shader->GetShader()), nullptr, 0);
			}
		}

		static void BindSampler(ID3D11SamplerState* sampler, eShaderType target_shader)
		{
			g_shader_sampler_bind_map.at(target_shader)(s_context_.Get(), sampler, static_cast<UINT>(target_shader), 1);
		}

		static void CreateSampler(const D3D11_SAMPLER_DESC& desc, ID3D11SamplerState** state)
		{
			s_device_->CreateSamplerState(&desc, state);
		}

		static void CreateBlendState(ID3D11BlendState** blend_state);
		static void CreateDepthStencilState(ID3D11DepthStencilState** depth_stencil_state);

		static void InitializeAdapter();
		static void InitializeDevice();
		static void InitializeRenderTargetView();
		static void InitializeDepthStencil();
		static void CreateRasterizer(ID3D11RasterizerState** state);

		inline static HWND s_hwnd_ = nullptr;

		inline static std::unique_ptr<D3Device> s_instance_ = nullptr;

		inline static ComPtr<ID3D11Device> s_device_ = nullptr;
		inline static ComPtr<ID3D11DeviceContext> s_context_ = nullptr;

		inline static UINT s_video_card_memory_ = 0;
		inline static UINT s_refresh_rate_numerator_ = 0;
		inline static UINT s_refresh_rate_denominator_ = 0;

		inline static DXGI_ADAPTER_DESC s_video_card_desc_ = {};

		inline static ComPtr<IDXGISwapChain> s_swap_chain_ = nullptr;
		inline static ComPtr<ID3D11RenderTargetView> s_render_target_view_ = nullptr;
		inline static ComPtr<ID3D11Texture2D> s_depth_stencil_buffer_ = nullptr;
		inline static ComPtr<ID3D11DepthStencilView> s_depth_stencil_view_ = nullptr;

		inline static D3D11_VIEWPORT s_viewport_{};

		inline static XMMATRIX s_world_matrix_ = {};
		inline static XMMATRIX s_projection_matrix_ = {};
		inline static XMMATRIX s_ortho_matrix_ = {};
	};
}
