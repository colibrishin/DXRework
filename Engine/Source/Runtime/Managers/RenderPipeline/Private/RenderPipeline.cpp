#include "../Public/RenderPipeline.h"
#include "Source/Runtime/Resources/Shader/Public/Shader.hpp"
#include "Source/Runtime/Managers/D3D12Wrapper/Public/D3Device.hpp"

namespace Engine::Managers
{
	using namespace Resources;

	void RenderPipeline::SetPerspectiveMatrix(const CBs::PerspectiveCB& matrix)
	{
		m_wvp_buffer_ = matrix;
		m_wvp_buffer_data_.SetData(&matrix);
	}

	void RenderPipeline::DefaultViewport(const Weak<CommandPair>& w_cmd) const
	{
		const auto& cmd = w_cmd.lock();
		cmd->GetList()->RSSetViewports(1, &m_viewport_);
	}

	void RenderPipeline::DefaultScissorRect(const Weak<CommandPair>& w_cmd) const
	{
		const auto& cmd = w_cmd.lock();
		cmd->GetList()->RSSetScissorRects(1, &m_scissor_rect_);
	}

	void RenderPipeline::DefaultRootSignature(const Weak<CommandPair>& w_cmd) const
	{
		const auto& cmd = w_cmd.lock();
		cmd->GetList()->SetGraphicsRootSignature(m_root_signature_.Get());
	}

	RenderPipeline::~RenderPipeline() { }

	void RenderPipeline::InitializeStaticBuffers()
	{
		m_wvp_buffer_data_.Create(nullptr);
		m_param_buffer_data_.Create(nullptr);
	}

	void RenderPipeline::InitializeViewport()
	{
		m_viewport_ = CD3DX12_VIEWPORT
				(
				 0.0f, 0.0f,
				 static_cast<float>(CFG_WIDTH),
				 static_cast<float>(CFG_HEIGHT),
				 0.0f, 1.0f
				);

		m_scissor_rect_ = CD3DX12_RECT
				(
				 0, 0,
				 static_cast<LONG>(CFG_WIDTH),
				 static_cast<LONG>(CFG_HEIGHT)
				);
	}

	void RenderPipeline::Initialize()
	{
		PrecompileShaders();
		InitializeRootSignature();
		InitializeHeaps();
		InitializeStaticBuffers();
		InitializeViewport();
		m_fallback_shader_ = Shader::Get("default").lock();
	}

	void RenderPipeline::PrecompileShaders()
	{
		Shader::Create
				(
				 "default", "./default.hlsl", SHADER_DOMAIN_OPAQUE,
				 SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
				 SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
				 D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				 SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS,
				 &g_default_rtv_format, 1, DXGI_FORMAT_D24_UNORM_S8_UINT,
				 D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
				);

		Shader::Create
				(
				 "color", "./color.hlsl", SHADER_DOMAIN_OPAQUE,
				 SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
				 SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
				 D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				 SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS,
				 &g_default_rtv_format, 1, DXGI_FORMAT_D24_UNORM_S8_UINT,
				 D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
				);

		Shader::Create
				(
				 "skybox", "./skybox.hlsl", SHADER_DOMAIN_OPAQUE,
				 SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
				 SHADER_RASTERIZER_CULL_NONE | SHADER_RASTERIZER_FILL_SOLID,
				 D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				 SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS,
				 &g_default_rtv_format, 1, DXGI_FORMAT_D24_UNORM_S8_UINT,
				 D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
				);

		Shader::Create
				(
				 "specular_normal", "./specular_normal.hlsl",
				 SHADER_DOMAIN_OPAQUE,
				 SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
				 SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
				 D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				 SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS,
				 &g_default_rtv_format, 1, DXGI_FORMAT_D24_UNORM_S8_UINT,
				 D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
				);

		Shader::Create
				(
				 "normal", "./normal.hlsl", SHADER_DOMAIN_OPAQUE,
				 SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
				 SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
				 D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				 SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS,
				 &g_default_rtv_format, 1, DXGI_FORMAT_D24_UNORM_S8_UINT,
				 D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
				);

		Shader::Create
				(
				 "refraction", "./refraction.hlsl", SHADER_DOMAIN_POST_PROCESS,
				 SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
				 SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
				 D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				 SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS,
				 &g_default_rtv_format, 1, DXGI_FORMAT_D24_UNORM_S8_UINT,
				 D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
				);

		Shader::Create
				(
				 "specular_tex", "./specular_tex.hlsl", SHADER_DOMAIN_OPAQUE,
				 SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
				 SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
				 D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				 SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS,
				 &g_default_rtv_format, 1, DXGI_FORMAT_D24_UNORM_S8_UINT,
				 D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
				);

		Shader::Create
				(
				 "specular", "./specular.hlsl", SHADER_DOMAIN_OPAQUE,
				 SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
				 SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
				 D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				 SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS,
				 &g_default_rtv_format, 1, DXGI_FORMAT_D24_UNORM_S8_UINT,
				 D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
				);

		Shader::Create
				(
				 "cascade_shadow_stage1", "./cascade_shadow_stage1.hlsl", SHADER_DOMAIN_OPAQUE,
				 SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
				 SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
				 D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				 SHADER_SAMPLER_CLAMP | SHADER_SAMPLER_ALWAYS,
				 &g_default_rtv_format, 1, DXGI_FORMAT_D32_FLOAT,
				 D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
				);

		const auto billboard = Shader::Create
				(
				 "billboard", "./billboard.hlsl", SHADER_DOMAIN_OPAQUE,
				 SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
				 SHADER_RASTERIZER_CULL_NONE | SHADER_RASTERIZER_FILL_SOLID,
				 D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				 SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS,
				 &g_default_rtv_format, 1, DXGI_FORMAT_D24_UNORM_S8_UINT,
				 D3D_PRIMITIVE_TOPOLOGY_POINTLIST, D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT
				);

		constexpr DXGI_FORMAT intensity_rtv_formats[]
		{
			DXGI_FORMAT_R32G32B32A32_UINT,
			DXGI_FORMAT_R32G32B32A32_FLOAT
		};

		Shader::Create
				(
				 "intensity_test", "./intensity_test.hlsl", SHADER_DOMAIN_OPAQUE,
				 SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
				 SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
				 D3D12_FILTER_MIN_MAG_MIP_POINT,
				 SHADER_SAMPLER_WRAP | SHADER_SAMPLER_NEVER,
				 intensity_rtv_formats, 2, DXGI_FORMAT_D32_FLOAT,
				 D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
				);

		Shader::Create
				(
				 "atlas", "./atlas.hlsl", SHADER_DOMAIN_OPAQUE,
				 SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
				 SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
				 D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				 SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS,
				 &g_default_rtv_format, 1, DXGI_FORMAT_D24_UNORM_S8_UINT,
				 D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
				);
	}

	void RenderPipeline::InitializeRootSignature()
	{
		CD3DX12_DESCRIPTOR_RANGE1 ranges[RASTERIZER_SLOT_COUNT];
		ranges[RASTERIZER_SLOT_SRV].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, g_max_engine_texture_slots, 0);
		ranges[RASTERIZER_SLOT_CB].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, g_max_cb_slots, 0);
		ranges[RASTERIZER_SLOT_UAV].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, g_max_uav_slots, 0);
		ranges[RASTERIZER_SLOT_SAMPLER].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, SAMPLER_END, 0);

		ranges[RASTERIZER_SLOT_SRV].Flags     = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
		ranges[RASTERIZER_SLOT_CB].Flags      = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
		ranges[RASTERIZER_SLOT_UAV].Flags     = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
		ranges[RASTERIZER_SLOT_SAMPLER].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;


		CD3DX12_ROOT_PARAMETER1 root_parameters[RASTERIZER_SLOT_COUNT];
		root_parameters[RASTERIZER_SLOT_SRV].InitAsDescriptorTable
				(1, &ranges[RASTERIZER_SLOT_SRV], D3D12_SHADER_VISIBILITY_ALL);
		root_parameters[RASTERIZER_SLOT_CB].InitAsDescriptorTable
				(1, &ranges[RASTERIZER_SLOT_CB], D3D12_SHADER_VISIBILITY_ALL);
		root_parameters[RASTERIZER_SLOT_UAV].InitAsDescriptorTable
				(1, &ranges[RASTERIZER_SLOT_UAV], D3D12_SHADER_VISIBILITY_ALL);
		root_parameters[RASTERIZER_SLOT_SAMPLER].InitAsDescriptorTable
				(1, &ranges[RASTERIZER_SLOT_SAMPLER], D3D12_SHADER_VISIBILITY_ALL);

		const CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc
				(
				 RASTERIZER_SLOT_COUNT,
				 root_parameters,
				 0,
				 nullptr,
				 D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
				);

		ComPtr<ID3DBlob> signature_blob = nullptr;
		ComPtr<ID3DBlob> error_blob     = nullptr;

		D3D12SerializeVersionedRootSignature
				(
				 &root_signature_desc,
				 signature_blob.GetAddressOf(),
				 error_blob.GetAddressOf()
				);

		if (error_blob)
		{
			const std::string error_message =
					static_cast<char*>(error_blob->GetBufferPointer());

			OutputDebugStringA(error_message.c_str());
		}

		DX::ThrowIfFailed
				(
				 Managers::D3Device::GetInstance().GetDevice()->CreateRootSignature
				 (
				  0,
				  signature_blob->GetBufferPointer(),
				  signature_blob->GetBufferSize(),
				  IID_PPV_ARGS(m_root_signature_.ReleaseAndGetAddressOf())
				 )
				);
	}

	void RenderPipeline::InitializeNullDescriptors()
	{
		// Dummy (null) descriptor
		constexpr D3D12_DESCRIPTOR_HEAP_DESC null_buffer_heap_desc
		{
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.NumDescriptors = 1,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			.NodeMask = 0
		};

		constexpr D3D12_DESCRIPTOR_HEAP_DESC null_sampler_heap_desc
		{
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
			.NumDescriptors = 1,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			.NodeMask = 0
		};

		constexpr D3D12_DESCRIPTOR_HEAP_DESC null_rtv_heap_desc
		{
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			.NumDescriptors = 1,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			.NodeMask = 0
		};

		constexpr D3D12_DESCRIPTOR_HEAP_DESC null_dsv_heap_desc
		{
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
			.NumDescriptors = 1,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			.NodeMask = 0
		};

		DX::ThrowIfFailed
				(
				 Managers::D3Device::GetInstance().GetDevice()->CreateDescriptorHeap
				 (
				  &null_buffer_heap_desc,
				  IID_PPV_ARGS(m_null_cbv_heap_.ReleaseAndGetAddressOf())
				 )
				);

		DX::ThrowIfFailed
				(
				 Managers::D3Device::GetInstance().GetDevice()->CreateDescriptorHeap
				 (
				  &null_buffer_heap_desc,
				  IID_PPV_ARGS(m_null_srv_heap_.ReleaseAndGetAddressOf())
				 )
				);

		DX::ThrowIfFailed
				(
				 Managers::D3Device::GetInstance().GetDevice()->CreateDescriptorHeap
				 (
				  &null_buffer_heap_desc,
				  IID_PPV_ARGS(m_null_uav_heap_.ReleaseAndGetAddressOf())
				 )
				);

		Managers::D3Device::GetInstance().GetDevice()->CreateConstantBufferView
				(
				 nullptr, m_null_cbv_heap_->GetCPUDescriptorHandleForHeapStart()
				);

		constexpr D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc
		{
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Texture2D = {0, 0, 0, 0}
		};

		Managers::D3Device::GetInstance().GetDevice()->CreateShaderResourceView
				(
				 nullptr, &srv_desc, m_null_srv_heap_->GetCPUDescriptorHandleForHeapStart()
				);

		constexpr D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc
		{
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
			.Texture2D = {0, 0,}
		};

		Managers::D3Device::GetInstance().GetDevice()->CreateUnorderedAccessView
				(
				 nullptr, nullptr, &uav_desc, m_null_uav_heap_->GetCPUDescriptorHandleForHeapStart()
				);

		DX::ThrowIfFailed
				(
				 Managers::D3Device::GetInstance().GetDevice()->CreateDescriptorHeap
				 (
				  &null_sampler_heap_desc,
				  IID_PPV_ARGS(m_null_sampler_heap_.ReleaseAndGetAddressOf())
				 )
				);

		DX::ThrowIfFailed
				(
				 Managers::D3Device::GetInstance().GetDevice()->CreateDescriptorHeap
				 (
				  &null_rtv_heap_desc,
				  IID_PPV_ARGS(m_null_rtv_heap_.ReleaseAndGetAddressOf())
				 )
				);

		DX::ThrowIfFailed
				(
				 Managers::D3Device::GetInstance().GetDevice()->CreateDescriptorHeap
				 (
				  &null_dsv_heap_desc,
				  IID_PPV_ARGS(m_null_dsv_heap_.ReleaseAndGetAddressOf())
				 )
				);

		constexpr D3D12_SAMPLER_DESC sampler_desc
		{
			.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
			.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			.MipLODBias = 0,
			.MaxAnisotropy = 0,
			.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
			.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
			.MinLOD = 0,
			.MaxLOD = D3D12_FLOAT32_MAX
		};

		Managers::D3Device::GetInstance().GetDevice()->CreateSampler
				(
				 &sampler_desc, m_null_sampler_heap_->GetCPUDescriptorHandleForHeapStart()
				);

		constexpr D3D12_RENDER_TARGET_VIEW_DESC rtv_desc
		{
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
			.Texture2D = {0, 0}
		};

		Managers::D3Device::GetInstance().GetDevice()->CreateRenderTargetView
				(
				 nullptr, &rtv_desc, m_null_rtv_heap_->GetCPUDescriptorHandleForHeapStart()
				);

		constexpr D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc
		{
			.Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
			.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
			.Flags = D3D12_DSV_FLAG_NONE,
			.Texture2D = {0}
		};

		Managers::D3Device::GetInstance().GetDevice()->CreateDepthStencilView
				(
				 nullptr, &dsv_desc, m_null_dsv_heap_->GetCPUDescriptorHandleForHeapStart()
				);
	}

	void RenderPipeline::InitializeHeaps()
	{
		m_buffer_descriptor_size_ = Managers::D3Device::GetInstance().GetDevice()->GetDescriptorHandleIncrementSize
				(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		m_sampler_descriptor_size_ = Managers::D3Device::GetInstance().GetDevice()->GetDescriptorHandleIncrementSize
				(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

		m_descriptor_handler_.Initialize(Managers::D3Device::GetInstance().GetDevice(), GetRootSignature());

		InitializeNullDescriptors();
	}

	void RenderPipeline::PreUpdate(const float& dt) {}

	void RenderPipeline::PreRender(const float& dt)
	{
		Managers::D3Device::GetInstance().ClearRenderTarget();
	}

	void RenderPipeline::Update(const float& dt) {}

	void RenderPipeline::Render(const float& dt) {}

	void RenderPipeline::FixedUpdate(const float& dt) {}

	void RenderPipeline::PostRender(const float& dt) {}

	void RenderPipeline::PostUpdate(const float& dt) {}

	ID3D12RootSignature* RenderPipeline::GetRootSignature() const
	{
		return m_root_signature_.Get();
	}

	D3D12_VIEWPORT RenderPipeline::GetViewport() const
	{
		return m_viewport_;
	}

	D3D12_RECT RenderPipeline::GetScissorRect() const
	{
		return m_scissor_rect_;
	}

	void RenderPipeline::SetPSO(const Weak<CommandPair>& w_cmd, const Strong<Shader>& shader)
	{
		const auto& cmd        = w_cmd.lock();

		D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_state_desc;

		const D3D12_INPUT_LAYOUT_DESC il
		{
			.pInputElementDescs = shader->GetInputElements().data(),
			.NumElements = static_cast<UINT>(shader->GetInputElementCount())
		};

		constexpr D3D12_SHADER_BYTECODE empty_shader = {nullptr, 0};

		pipeline_state_desc.pRootSignature = m_root_signature_.Get();
		pipeline_state_desc.InputLayout    = il;
		pipeline_state_desc.VS             = {shader->GetVSBlob()->GetBufferPointer(), shader->GetVSBlob()->GetBufferSize()};
		pipeline_state_desc.PS             = {shader->GetPSBlob()->GetBufferPointer(), shader->GetPSBlob()->GetBufferSize()};

		if (ID3DBlob* gs = shader->GetGSBlob())
		{
			pipeline_state_desc.GS = {gs->GetBufferPointer(), gs->GetBufferSize()};
		}
		else
		{
			pipeline_state_desc.GS = empty_shader;
		}

		if (ID3DBlob* hs = shader->GetHSBlob())
		{
			pipeline_state_desc.HS = {hs->GetBufferPointer(), hs->GetBufferSize()};
		}
		else
		{
			pipeline_state_desc.HS = empty_shader;
		}

		if (ID3DBlob* ds = shader->GetDSBlob())
		{
			pipeline_state_desc.DS = {ds->GetBufferPointer(), ds->GetBufferSize()};
		}
		else
		{
			pipeline_state_desc.DS = empty_shader;
		}

		pipeline_state_desc.SampleDesc            = {1, 0};
		pipeline_state_desc.PrimitiveTopologyType = shader->GetTopologyType();
		pipeline_state_desc.RasterizerState       = shader->GetRasterizerDesc();
		pipeline_state_desc.DepthStencilState     = shader->GetDepthStencilDesc();
		pipeline_state_desc.SampleMask            = UINT_MAX;
		pipeline_state_desc.BlendState            = shader->GetBlendDesc();
		pipeline_state_desc.NodeMask              = 0;
		pipeline_state_desc.CachedPSO             = {};
		pipeline_state_desc.Flags                 = D3D12_PIPELINE_STATE_FLAG_NONE;
		pipeline_state_desc.DSVFormat             = shader->GetDSVFormat();
		pipeline_state_desc.NumRenderTargets      = static_cast<UINT>(shader->GetRTVFormats().size());

		const std::vector<DXGI_FORMAT> formats = shader->GetRTVFormats();

		for (size_t i = 0; i < formats.size(); ++i)
		{
			pipeline_state_desc.RTVFormats[i] = formats[i];
		}

		// todo: cache PSO for each shaders

		DX::ThrowIfFailed
				(
				 Managers::D3Device::GetInstance().GetDevice()->CreateGraphicsPipelineState
				 (
				  &pipeline_state_desc,
				  IID_PPV_ARGS(m_pipeline_state_.ReleaseAndGetAddressOf())
				 )
				);

		cmd->GetList()->SetPipelineState(m_pipeline_state_.Get());
	}

	DescriptorPtr RenderPipeline::AcquireHeapSlot()
	{
		std::lock_guard<std::mutex> lock(m_descriptor_mutex_);
		return m_descriptor_handler_.Acquire();
	}

	UINT RenderPipeline::GetBufferDescriptorSize() const
	{
		return m_buffer_descriptor_size_;
	}

	UINT RenderPipeline::GetSamplerDescriptorSize() const
	{
		return m_sampler_descriptor_size_;
	}

	void RenderPipeline::BindConstantBuffers(const Weak<CommandPair>& w_cmd, const DescriptorPtr& heap)
	{
		const auto& cmd = w_cmd.lock();
		m_wvp_buffer_data_.Bind(cmd, heap);
		m_param_buffer_data_.Bind(cmd, heap);
	}

	void RenderPipeline::BindConstantBuffers(ID3D12GraphicsCommandList1* cmd, const DescriptorPtr& heap)
	{
		m_wvp_buffer_data_.Bind(cmd, heap);
		m_param_buffer_data_.Bind(cmd, heap);
	}
} // namespace Engine::Manager::Graphics