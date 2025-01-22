#include "../Public/Shader.hpp"
#include "Source/Runtime/Managers/ResourceManager/Public/ResourceManager.hpp"

#include <ranges>

#if defined(USE_DX12)
#include <d3dcompiler.h>
#include "Source/Runtime/Managers/D3D12Wrapper/Public/D3Device.hpp"
#include "Source/Runtime/ThrowIfFailed/Public/ThrowIfFailed.h"
#endif

namespace Engine::Resources
{
	void Shader::Load_INTERNAL()
	{
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
					 GetPath().c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
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
					m_il_      = Managers::D3Device::GetInstance().GenerateInputDescription(blob.Get());
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

		m_depth_stencil_desc_.DepthEnable                  = m_depth_flag_;
		m_depth_stencil_desc_.DepthWriteMask               = m_depth_test_;
		m_depth_stencil_desc_.DepthFunc                    = m_depth_func_;
		m_depth_stencil_desc_.StencilEnable                = true;
		m_depth_stencil_desc_.StencilReadMask              = 0xFF;
		m_depth_stencil_desc_.StencilWriteMask             = 0xFF;
		m_depth_stencil_desc_.FrontFace.StencilFailOp      = D3D12_STENCIL_OP_KEEP;
		m_depth_stencil_desc_.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_INCR;
		m_depth_stencil_desc_.FrontFace.StencilPassOp      = D3D12_STENCIL_OP_KEEP;
		m_depth_stencil_desc_.FrontFace.StencilFunc        = D3D12_COMPARISON_FUNC_ALWAYS;
		m_depth_stencil_desc_.BackFace.StencilFailOp       = D3D12_STENCIL_OP_KEEP;
		m_depth_stencil_desc_.BackFace.StencilDepthFailOp  = D3D12_STENCIL_OP_DECR;
		m_depth_stencil_desc_.BackFace.StencilPassOp       = D3D12_STENCIL_OP_KEEP;
		m_depth_stencil_desc_.BackFace.StencilFunc         = D3D12_COMPARISON_FUNC_ALWAYS;

		m_rasterizer_desc_.AntialiasedLineEnable = true;
		m_rasterizer_desc_.DepthBias             = 0;
		m_rasterizer_desc_.DepthBiasClamp        = 0.0f;
		m_rasterizer_desc_.DepthClipEnable       = true;
		m_rasterizer_desc_.CullMode              = m_cull_mode_;
		m_rasterizer_desc_.FillMode              = m_fill_mode_;
		m_rasterizer_desc_.FrontCounterClockwise = false;
		m_rasterizer_desc_.MultisampleEnable     = false;
		m_rasterizer_desc_.ForcedSampleCount     = 0;
		m_rasterizer_desc_.SlopeScaledDepthBias  = 0.0f;
		m_rasterizer_desc_.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

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

		m_blend_desc_.AlphaToCoverageEnable  = m_domain_ == SHADER_DOMAIN_TRANSPARENT ? true : false;
		m_blend_desc_.IndependentBlendEnable = false;

		for (int i = 0; i < m_rtv_formats_.size(); ++i)
		{
			m_blend_desc_.RenderTarget[i].BlendEnable           = m_domain_ == SHADER_DOMAIN_TRANSPARENT ? true : false;
			m_blend_desc_.RenderTarget[i].LogicOpEnable         = false;
			m_blend_desc_.RenderTarget[i].SrcBlend              = D3D12_BLEND_SRC_ALPHA;
			m_blend_desc_.RenderTarget[i].DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
			m_blend_desc_.RenderTarget[i].BlendOp               = D3D12_BLEND_OP_ADD;
			m_blend_desc_.RenderTarget[i].SrcBlendAlpha         = D3D12_BLEND_ONE;
			m_blend_desc_.RenderTarget[i].DestBlendAlpha        = D3D12_BLEND_ZERO;
			m_blend_desc_.RenderTarget[i].BlendOpAlpha          = D3D12_BLEND_OP_ADD;
			m_blend_desc_.RenderTarget[i].LogicOp               = D3D12_LOGIC_OP_NOOP;
			m_blend_desc_.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		}

		Managers::D3Device::GetInstance().GetDevice()->CreateSampler(&sd, m_sampler_descriptor_heap_->GetCPUDescriptorHandleForHeapStart());

		m_il_elements_.reserve(m_il_.size());

		for (const auto& element : m_il_ | std::views::keys)
		{
			m_il_elements_.push_back(element);
		}
	}

	Shader::Shader(
		const EntityName&      name, const std::filesystem::path& path, const eShaderDomain domain,
		const UINT             depth, const UINT rasterizer, const D3D12_FILTER filter, const UINT sampler,
		const DXGI_FORMAT*     rtv_format, const UINT rtv_count, DXGI_FORMAT dsv_format,
		D3D_PRIMITIVE_TOPOLOGY topology, D3D12_PRIMITIVE_TOPOLOGY_TYPE topology_type
	)
		: Resource(path, RES_T_SHADER),
		  m_domain_(domain),
		  m_depth_flag_(depth != 0),
		  m_depth_test_(static_cast<D3D12_DEPTH_WRITE_MASK>(depth & 1)),
		  m_depth_func_(static_cast<D3D12_COMPARISON_FUNC>(std::log2(depth >> 1) + 1)),
		  m_smp_filter_(filter),
		  m_smp_address_(static_cast<D3D12_TEXTURE_ADDRESS_MODE>((sampler & shader_sampler_address_mask) + 1)),
		  m_smp_func_(static_cast<D3D12_COMPARISON_FUNC>(std::log2(sampler >> 3))),
		  m_cull_mode_(static_cast<D3D12_CULL_MODE>((rasterizer & 2) + 1)),
		  m_fill_mode_(static_cast<D3D12_FILL_MODE>((rasterizer >> 2) + 1)),
		  m_dsv_format_(dsv_format),
		  m_topology_(topology),
		  m_topology_type_(topology_type)
	{
		SetName(name);

		for (UINT i = 0; i < rtv_count; ++i)
		{
			m_rtv_formats_.push_back(rtv_format[i]);
		}
	}

	void Shader::Initialize() {}

	void Shader::PreUpdate(const float& dt) {}

	void Shader::Update(const float& dt) {}

	void Shader::FixedUpdate(const float& dt) {}

	void Shader::PostUpdate(const float& dt) {}

	void Shader::Unload_INTERNAL()
	{
		m_vs_blob_.Reset();
		m_ps_blob_.Reset();
		m_gs_blob_.Reset();
		m_hs_blob_.Reset();
		m_ds_blob_.Reset();
		m_il_.clear();
		m_il_elements_.clear();
		m_sampler_descriptor_heap_.Reset();
		m_pipeline_state_.Reset();
	}

	void Shader::OnDeserialized()
	{
		Resource::OnDeserialized();
	}

	void Shader::SetTopology(D3D_PRIMITIVE_TOPOLOGY topology, D3D12_PRIMITIVE_TOPOLOGY_TYPE type)
	{
		m_topology_      = topology;
		m_topology_type_ = type;
	}

	eShaderDomain Shader::GetDomain() const
	{
		return m_domain_;
	}

	ID3D12PipelineState* Shader::GetPipelineState() const
	{
		return m_pipeline_state_.Get();
	}

	D3D_PRIMITIVE_TOPOLOGY Shader::GetTopology() const
	{
		return m_topology_;
	}

	D3D12_PRIMITIVE_TOPOLOGY_TYPE Shader::GetTopologyType() const
	{
		return m_topology_type_;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Shader::GetShaderHeap() const
	{
		return m_sampler_descriptor_heap_->GetCPUDescriptorHandleForHeapStart();
	}

	ID3DBlob* Shader::GetVSBlob() const
	{
		return m_vs_blob_.Get();
	}

	ID3DBlob* Shader::GetPSBlob() const
	{
		return m_ps_blob_.Get();
	}

	ID3DBlob* Shader::GetGSBlob() const
	{
		return m_gs_blob_.Get();
	}

	ID3DBlob* Shader::GetHSBlob() const
	{
		return m_hs_blob_.Get();
	}

	ID3DBlob* Shader::GetDSBlob() const
	{
		return m_ds_blob_.Get();
	}

	D3D12_RASTERIZER_DESC Shader::GetRasterizerDesc() const
	{
		return m_rasterizer_desc_;
	}

	D3D12_DEPTH_STENCIL_DESC Shader::GetDepthStencilDesc() const
	{
		return m_depth_stencil_desc_;
	}

	D3D12_BLEND_DESC Shader::GetBlendDesc() const
	{
		return m_blend_desc_;
	}

	const std::vector<DXGI_FORMAT>& Shader::GetRTVFormats() const
	{
		return m_rtv_formats_;
	}

	DXGI_FORMAT Shader::GetDSVFormat() const
	{
		return m_dsv_format_;
	}

	const std::vector<D3D12_INPUT_ELEMENT_DESC>& Shader::GetInputElements() const
	{
		return m_il_elements_;
	}

	size_t Shader::GetInputElementCount() const
	{
		return m_il_.size();
	}

	boost::weak_ptr<Shader> Shader::Get(const std::string& name)
	{
		return Managers::ResourceManager::GetInstance().GetResource<Shader>(name);
	}

	boost::shared_ptr<Shader> Shader::Create(
		const std::string&     name, const std::filesystem::path& path, const eShaderDomain domain, const UINT depth,
		const UINT             rasterizer, const D3D12_FILTER filter, const UINT sampler,
		const DXGI_FORMAT*     rtv_format, const UINT rtv_count, const DXGI_FORMAT dsv_format,
		D3D_PRIMITIVE_TOPOLOGY topology, D3D12_PRIMITIVE_TOPOLOGY_TYPE topology_type
	)
	{
		if (const auto pcheck = Managers::ResourceManager::GetInstance().GetResourceByRawPath<Shader>
					(path).lock();
			const auto ncheck = Managers::ResourceManager::GetInstance().GetResource<Shader>(name).lock())
		{
			return ncheck;
		}

		const auto obj = boost::make_shared<Shader>
				(
				 name, path, domain, depth, rasterizer, filter, sampler, rtv_format, rtv_count, dsv_format, topology,
				 topology_type
				);
		Managers::ResourceManager::GetInstance().AddResource(name, obj);
		return obj;
	}

	void Shader::OnSerialized()
	{
		if (exists(GetPath()))
		{
			const std::filesystem::path folder   = GetPrettyTypeName();
			const std::filesystem::path filename = GetPath().filename();
			const std::filesystem::path p        = folder / filename;

			if (!exists(folder))
			{
				create_directory(folder);
			}

			if (GetPath() == p)
			{
				return;
			}

			if (exists(p))
			{
				std::filesystem::remove(p);
			}

			copy_file(GetPath(), p, std::filesystem::copy_options::overwrite_existing);

			SetPath(p);
		}
	}

	Shader::Shader()
		: Resource("", RES_T_SHADER),
		  m_domain_(),
		  m_depth_flag_(false),
		  m_depth_test_(),
		  m_depth_func_(),
		  m_smp_filter_(),
		  m_smp_address_(),
		  m_smp_func_(),
		  m_cull_mode_(),
		  m_fill_mode_(),
		  m_rtv_formats_(DXGI_FORMAT_R8G8B8A8_UNORM),
		  m_dsv_format_(DXGI_FORMAT_D24_UNORM_S8_UINT),
		  m_topology_(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST),
		  m_topology_type_(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE) { }
} // namespace Engine::Graphic
