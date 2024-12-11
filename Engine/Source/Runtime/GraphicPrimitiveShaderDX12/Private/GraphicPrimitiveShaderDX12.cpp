#include "GraphicPrimitiveShaderDX12.h"
#include <ranges>
#include <d3dcompiler.h>
#include "Source/Runtime/Managers/D3D12Wrapper/Public/D3Device.hpp"
#include "Source/Runtime/Resources/Shader/Public/Shader.hpp"
#include "Source/Runtime/ThrowIfFailed/Public/ThrowIfFailed.h"

std::vector<std::tuple<Engine::eShaderType, std::string, std::string>> Engine::DX12GraphicPrimitiveShader::s_main_version =
		{
			{SHADER_VERTEX, "vs_main", "vs_5_0"},
			{SHADER_PIXEL, "ps_main", "ps_5_0"},
			{SHADER_GEOMETRY, "gs_main", "gs_5_0"},
			{SHADER_COMPUTE, "cs_main", "cs_5_0"},
			{SHADER_HULL, "hs_main", "hs_5_0"},
			{SHADER_DOMAIN, "ds_main", "ds_5_0"}
		};

namespace Engine
{
	void DX12GraphicPrimitiveShader::Generate(const Weak<Resources::Shader>& w_shader, void* pipeline_signature)
	{
		if (const Strong<Resources::Shader>& shader = w_shader.lock())
		{
			ConvertShader(shader.get());

			ComPtr<ID3DBlob> blob;
			ComPtr<ID3DBlob> error;
			UINT             flag = 0;

#if WITH_DEBUG
			flag |= D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG;
#endif

			for (const auto& [t, ep, v] : s_main_version)
			{
				// Try search for every type of shader.
				const auto res = D3DCompileFromFile
						(
						 shader->GetPath().c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
						 ep.c_str(), v.c_str(), flag, 0,
						 &blob, &error
						);

				// Print the warnings if there were.
				if (error)
				{
					const std::string error_message =
							static_cast<char*>(error->GetBufferPointer());

					// Silencing the no entry point error.
					if (error_message.find("X3501") == std::string::npos)
					{
						OutputDebugStringA(error_message.c_str());
					}
				}


				if (res == S_OK)
				{
					// If shader is vertex shader, generate input layout.
					if (t == SHADER_VERTEX)
					{
						m_il_      = GenerateInputDescription(blob.Get());
						m_vs_blob_ = blob;
					}
					else if (t == SHADER_PIXEL)
					{
						m_ps_blob_ = blob;
					}
					else if (t == SHADER_GEOMETRY)
					{
						m_gs_blob_ = blob;
					}
					else if (t == SHADER_HULL)
					{
						m_hs_blob_ = blob;
					}
					else if (t == SHADER_DOMAIN)
					{
						m_ds_blob_ = blob;
					}
				}
			}

			if (!m_vs_blob_)
			{
				throw std::runtime_error("Vertex shader is not found");
			}

			D3D12_DEPTH_STENCIL_DESC dsd;
			dsd.DepthEnable                  = m_depth_flag_;
			dsd.DepthWriteMask               = m_depth_test_;
			dsd.DepthFunc                    = m_depth_func_;
			dsd.StencilEnable                = true;
			dsd.StencilReadMask              = 0xFF;
			dsd.StencilWriteMask             = 0xFF;
			dsd.FrontFace.StencilFailOp      = D3D12_STENCIL_OP_KEEP;
			dsd.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_INCR;
			dsd.FrontFace.StencilPassOp      = D3D12_STENCIL_OP_KEEP;
			dsd.FrontFace.StencilFunc        = D3D12_COMPARISON_FUNC_ALWAYS;
			dsd.BackFace.StencilFailOp       = D3D12_STENCIL_OP_KEEP;
			dsd.BackFace.StencilDepthFailOp  = D3D12_STENCIL_OP_DECR;
			dsd.BackFace.StencilPassOp       = D3D12_STENCIL_OP_KEEP;
			dsd.BackFace.StencilFunc         = D3D12_COMPARISON_FUNC_ALWAYS;

			D3D12_RASTERIZER_DESC rd;
			rd.AntialiasedLineEnable = true;
			rd.DepthBias             = 0;
			rd.DepthBiasClamp        = 0.0f;
			rd.DepthClipEnable       = true;
			rd.CullMode              = m_cull_mode_;
			rd.FillMode              = m_fill_mode_;
			rd.FrontCounterClockwise = false;
			rd.MultisampleEnable     = false;
			rd.ForcedSampleCount     = 0;
			rd.SlopeScaledDepthBias  = 0.0f;
			rd.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

			constexpr D3D12_DESCRIPTOR_HEAP_DESC dhd =
			{
				.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
				.NumDescriptors = 1,
				.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
				.NodeMask = 0
			};

			DX::ThrowIfFailed
					(
					 Managers::D3Device::GetInstance().GetDevice()->CreateDescriptorHeap
					 (
					  &dhd,
					  IID_PPV_ARGS(m_sampler_descriptor_heap_.ReleaseAndGetAddressOf())
					 )
					);

			D3D12_SAMPLER_DESC sd;
			sd.Filter         = m_smp_filter_;
			sd.AddressU       = m_smp_address_;
			sd.AddressV       = m_smp_address_;
			sd.AddressW       = m_smp_address_;
			sd.MipLODBias     = 0.0f;
			sd.MaxAnisotropy  = 1;
			sd.ComparisonFunc = m_smp_func_;
			sd.BorderColor[0] = 0.0f;
			sd.BorderColor[1] = 0.0f;
			sd.BorderColor[2] = 0.0f;
			sd.BorderColor[3] = 0.0f;
			sd.MinLOD         = 0.0f;
			sd.MaxLOD         = D3D12_FLOAT32_MAX;

			D3D12_BLEND_DESC bd;
			bd.AlphaToCoverageEnable  = m_domain_ == SHADER_DOMAIN_TRANSPARENT ? true : false;
			bd.IndependentBlendEnable = false;

			for (int i = 0; i < m_rtv_formats_.size(); ++i)
			{
				bd.RenderTarget[i].BlendEnable           = m_domain_ == SHADER_DOMAIN_TRANSPARENT ? true : false;
				bd.RenderTarget[i].LogicOpEnable         = false;
				bd.RenderTarget[i].SrcBlend              = D3D12_BLEND_SRC_ALPHA;
				bd.RenderTarget[i].DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
				bd.RenderTarget[i].BlendOp               = D3D12_BLEND_OP_ADD;
				bd.RenderTarget[i].SrcBlendAlpha         = D3D12_BLEND_ONE;
				bd.RenderTarget[i].DestBlendAlpha        = D3D12_BLEND_ZERO;
				bd.RenderTarget[i].BlendOpAlpha          = D3D12_BLEND_OP_ADD;
				bd.RenderTarget[i].LogicOp               = D3D12_LOGIC_OP_NOOP;
				bd.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			}

			Managers::D3Device::GetInstance().GetDevice()->CreateSampler(&sd, m_sampler_descriptor_heap_->GetCPUDescriptorHandleForHeapStart());

			m_il_elements_.reserve(m_il_.size());

			for (const auto& element : m_il_ | std::views::keys)
			{
				m_il_elements_.push_back(element);
			}

			const D3D12_INPUT_LAYOUT_DESC il
			{
				.pInputElementDescs = m_il_elements_.data(),
				.NumElements = static_cast<UINT>(m_il_.size())
			};

			constexpr D3D12_SHADER_BYTECODE empty_shader = {nullptr, 0};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_state_desc;

			pipeline_state_desc.pRootSignature = static_cast<ID3D12RootSignature*>(pipeline_signature);
			pipeline_state_desc.InputLayout    = il;
			pipeline_state_desc.VS             = {m_vs_blob_->GetBufferPointer(), m_vs_blob_->GetBufferSize()};
			pipeline_state_desc.PS             = {m_ps_blob_->GetBufferPointer(), m_ps_blob_->GetBufferSize()};

			if (m_gs_blob_)
			{
				pipeline_state_desc.GS = {m_gs_blob_->GetBufferPointer(), m_gs_blob_->GetBufferSize()};
			}
			else
			{
				pipeline_state_desc.GS = empty_shader;
			}

			if (m_hs_blob_)
			{
				pipeline_state_desc.HS = {m_hs_blob_->GetBufferPointer(), m_hs_blob_->GetBufferSize()};
			}
			else
			{
				pipeline_state_desc.HS = empty_shader;
			}

			if (m_ds_blob_)
			{
				pipeline_state_desc.DS = {m_ds_blob_->GetBufferPointer(), m_ds_blob_->GetBufferSize()};
			}
			else
			{
				pipeline_state_desc.DS = empty_shader;
			}

			pipeline_state_desc.SampleDesc            = {1, 0};
			pipeline_state_desc.PrimitiveTopologyType = m_topology_type_;
			pipeline_state_desc.RasterizerState       = rd;
			pipeline_state_desc.DepthStencilState     = dsd;
			pipeline_state_desc.SampleMask            = UINT_MAX;
			pipeline_state_desc.BlendState            = bd;
			pipeline_state_desc.NodeMask              = 0;
			pipeline_state_desc.CachedPSO             = {};
			pipeline_state_desc.Flags                 = D3D12_PIPELINE_STATE_FLAG_NONE;
			pipeline_state_desc.DSVFormat             = m_dsv_format_;
			pipeline_state_desc.NumRenderTargets      = static_cast<UINT>(m_rtv_formats_.size());

			for (size_t i = 0; i < m_rtv_formats_.size(); ++i)
			{
				pipeline_state_desc.RTVFormats[i] = m_rtv_formats_[i];
			}

			DX::ThrowIfFailed
					(
					 Managers::D3Device::GetInstance().GetDevice()->CreateGraphicsPipelineState
					 (
					  &pipeline_state_desc,
					  IID_PPV_ARGS(m_pipeline_state_.ReleaseAndGetAddressOf())
					 )
					);
		}
	}

	std::vector<std::pair<D3D12_INPUT_ELEMENT_DESC, std::string>> DX12GraphicPrimitiveShader::GenerateInputDescription(
		ID3DBlob* blob
	)
	{
		ComPtr<ID3D12ShaderReflection> reflection = nullptr;

		DX::ThrowIfFailed
				(
				 D3DReflect
				 (
				  blob->GetBufferPointer(), blob->GetBufferSize(),
				  IID_ID3D11ShaderReflection,
				  reinterpret_cast<void**>(reflection.GetAddressOf())
				 )
				);

		std::vector<std::pair<D3D12_INPUT_ELEMENT_DESC, std::string>> input_descs_with_name;

		D3D12_SHADER_DESC desc{};
		DX::ThrowIfFailed(reflection->GetDesc(&desc));

		UINT byteOffset = 0;

		for (UINT i = 0; i < desc.InputParameters; ++i)
		{
			D3D12_SIGNATURE_PARAMETER_DESC param_desc;
			D3D12_INPUT_ELEMENT_DESC       input_desc{};
			DX::ThrowIfFailed(reflection->GetInputParameterDesc(i, &param_desc));

			std::string name_buffer(param_desc.SemanticName);

			input_desc.SemanticName         = name_buffer.c_str();
			input_desc.SemanticIndex        = param_desc.SemanticIndex;
			input_desc.InputSlot            = 0;
			input_desc.AlignedByteOffset    = byteOffset;
			input_desc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			input_desc.InstanceDataStepRate = 0;

			// determine DXGI format
			if (param_desc.Mask == 1)
			{
				if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				{
					input_desc.Format = DXGI_FORMAT_R32_UINT;
				}
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				{
					input_desc.Format = DXGI_FORMAT_R32_SINT;
				}
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
				{
					input_desc.Format = DXGI_FORMAT_R32_FLOAT;
				}
				byteOffset += 4;
			}
			else if (param_desc.Mask <= 3)
			{
				if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				{
					input_desc.Format = DXGI_FORMAT_R32G32_UINT;
				}
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				{
					input_desc.Format = DXGI_FORMAT_R32G32_SINT;
				}
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
				{
					input_desc.Format = DXGI_FORMAT_R32G32_FLOAT;
				}
				byteOffset += 8;
			}
			else if (param_desc.Mask <= 7)
			{
				if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				{
					input_desc.Format = DXGI_FORMAT_R32G32B32_UINT;
				}
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				{
					input_desc.Format = DXGI_FORMAT_R32G32B32_SINT;
				}
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
				{
					input_desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
				}
				byteOffset += 12;
			}
			else if (param_desc.Mask <= 15)
			{
				if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				{
					input_desc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
				}
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				{
					input_desc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
				}
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
				{
					input_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				}
				byteOffset += 16;
			}

			input_descs_with_name.emplace_back(input_desc, name_buffer);
		}

		for (auto& dsc : input_descs_with_name)
		{
			dsc.first.SemanticName = dsc.second.c_str();
		}

		return input_descs_with_name;
	}

	ID3D12PipelineState* DX12GraphicPrimitiveShader::GetPSO() const
	{
		return m_pipeline_state_.Get();
	}

	ID3D12DescriptorHeap* DX12GraphicPrimitiveShader::GetSamplerHeap() const
	{
		return m_sampler_descriptor_heap_.Get();
	}

	void DX12GraphicPrimitiveShader::ConvertShader(const Resources::Shader* shader)
	{
		m_domain_ = shader->GetDomain();
		m_depth_flag_ = shader->GetDepth() != 0;
		m_depth_test_ = static_cast<D3D12_DEPTH_WRITE_MASK>(shader->GetDepth() & 1);
		m_depth_func_ = static_cast<D3D12_COMPARISON_FUNC>(std::log2(shader->GetDepth() >> 1) + 1);
		m_smp_filter_ = static_cast<D3D12_FILTER>(shader->GetSamplerFilter());
		m_smp_func_ = static_cast<D3D12_COMPARISON_FUNC>(shader->GetShaderSampler());
		m_cull_mode_ = static_cast<D3D12_CULL_MODE>((shader->GetRasterizer() & 2) + 1);
		m_fill_mode_ = static_cast<D3D12_FILL_MODE>((shader->GetRasterizer() >> 2) + 1);

		for (const eFormat format : shader->GetRTVFormat())
		{
			m_rtv_formats_.push_back(static_cast<DXGI_FORMAT>(format));
		}

		m_dsv_format_ = static_cast<DXGI_FORMAT>(shader->GetDSVFormat());
		m_topology_ = static_cast<D3D_PRIMITIVE_TOPOLOGY>(shader->GetPrimitiveTopology());
		m_topology_type_ = static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(shader->GetPrimitiveTopologyType());
	}
}
