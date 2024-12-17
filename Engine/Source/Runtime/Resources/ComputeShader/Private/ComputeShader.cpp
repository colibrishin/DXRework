#include "../Public/ComputeShader.h"

#include <numeric>

#include <d3dcompiler.h>
#include <directx/d3d12.h>

#include "Source/Runtime/Managers/D3D12Wrapper/Public/StructuredBufferDX12.hpp"
#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"
#if WITH_DEBUG
#include "Source/Runtime/Managers/Debugger/Public/Debugger.hpp"
#endif
#include "Source/Runtime/Managers/D3D12Wrapper/Public/D3Device.hpp"
#include "Source/Runtime/ThrowIfFailed/Public/ThrowIfFailed.h"

#include "Source/Runtime/Managers/ResourceManager/Public/ResourceManager.hpp"

namespace Engine::Resources
{
	RESOURCE_SELF_INFER_GETTER_IMPL(ComputeShader);

	void ComputeShader::Dispatch(
		ID3D12GraphicsCommandList1* list, 
		const DescriptorPtr& w_heap, 
		Graphics::SBs::LocalParamSB& param, 
		Graphics::StructuredBuffer<Graphics::SBs::LocalParamSB>&     buffer
	)
	{
		preDispatch(list, w_heap, param);

		list->SetComputeRootSignature(Managers::RenderPipeline::GetInstance().GetPrimitivePipeline()->GetNativePipeline());
		buffer.SetData(list, 1, &param);
		buffer.TransitionToSRV(list);
		buffer.CopySRVHeap(w_heap);

		Managers::RenderPipeline::GetInstance().BindConstantBuffers(list, w_heap);

		if (const auto& heap = w_heap.lock())
		{
			heap->BindCompute(list);
		}

		if (std::accumulate(m_group_, m_group_ + 3, 0) == 0)
		{
#if WITH_DEBUG
			Managers::Debugger::GetInstance().Log("ComputeShader::Dispatch() : Group is not set. Ignore dispatching...");
#endif
			return;
		}

		if (std::accumulate(m_group_, m_group_ + 3, 1, std::multiplies()) > 1 << 16)
		{
#if WITH_DEBUG
			Managers::Debugger::GetInstance().Log("ComputeShader::Dispatch() : Group is too large. Ignore dispatching...");

#endif // WITH_DEBUG

			return;
		}

		if (std::accumulate(m_thread_, m_thread_ + 3, 0) == 0)
		{
#if WITH_DEBUG
			Managers::Debugger::GetInstance().Log("ComputeShader::Dispatch() : Thread is not set. Ignore dispatching...");

#endif // WITH_DEBUG
			return;
		}

		if (std::accumulate(m_thread_, m_thread_ + 3, 1, std::multiplies()) > 1024)
		{
#if WITH_DEBUG
			//Managers::Debugger::GetInstance().Log("ComputeShader::Dispatch() : Thread is too large. Ignore dispatching...");

#endif // WITH_DEBUG
			return;
		}

		list->SetPipelineState(m_pipeline_state_.Get());

		list->Dispatch(m_group_[0], m_group_[1], m_group_[2]);

		buffer.TransitionCommon(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

		postDispatch(list, w_heap, param);

		std::fill_n(m_group_, 3, 1);
	}

	ComputeShader::ComputeShader(
		const std::string&           name,
		const std::filesystem::path& path,
		const std::array<UINT, 3>&   thread
	)
		: Shader
		(
		 name, path, SHADER_DOMAIN_OPAQUE, 0, SHADER_RASTERIZER_CULL_NONE,
		 D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT, SHADER_SAMPLER_NEVER,
		 &g_default_rtv_format, 1, DXGI_FORMAT_D24_UNORM_S8_UINT
		)
	{
		SetPath(path);
		std::ranges::copy(thread, m_thread_);
	}

	void ComputeShader::SetGroup(const std::array<UINT, 3>& group)
	{
		std::ranges::copy(group, m_group_);
	}

	std::array<UINT, 3> ComputeShader::GetThread() const
	{
		std::array<UINT, 3> thread;
		thread[0] = m_thread_[0];
		thread[1] = m_thread_[1];
		thread[2] = m_thread_[2];
		return thread;
	}

	void ComputeShader::PostUpdate(const float& dt) {}

	void ComputeShader::PreUpdate(const float& dt) {}

	void ComputeShader::FixedUpdate(const float& dt) {}

	void ComputeShader::Update(const float& dt) {}

	void ComputeShader::Initialize()
	{
		Shader::Initialize();
		std::fill_n(m_group_, 3, 1);
	}

	void ComputeShader::Load_INTERNAL()
	{
		ComPtr<ID3DBlob> error;
		UINT             flag = 0;

#if WITH_DEBUG
			flag |= D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG;
#endif

		const auto [type, entry, version] = s_main_version[SHADER_COMPUTE];

		const auto res = D3DCompileFromFile
				(
				 GetPath().c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
				 entry.c_str(), version.c_str(), flag, 0,
				 &m_cs_, &error
				);

		if (error)
		{
			const std::string error_message =
					static_cast<char*>(error->GetBufferPointer());
			OutputDebugStringA(error_message.c_str());
		}

		if (FAILED(res))
		{
			throw std::exception("ComputeShader::Load_INTERNAL() : Failed to compile shader.");
		}

		CD3DX12_PIPELINE_STATE_STREAM_CS cs_stream
		{
			CD3DX12_SHADER_BYTECODE(m_cs_.Get())
		};

		const D3D12_COMPUTE_PIPELINE_STATE_DESC desc
		{
			Managers::RenderPipeline::GetInstance().GetRootSignature(),
			cs_stream,
			0,
			D3D12_CACHED_PIPELINE_STATE{nullptr, 0},
			D3D12_PIPELINE_STATE_FLAG_NONE
		};

		DX::ThrowIfFailed
				(
				 Managers::D3Device::GetInstance().GetDevice()->CreateComputePipelineState
				 (
				  &desc, IID_PPV_ARGS(m_pipeline_state_.GetAddressOf())
				 )
				);

		loadDerived();
	}

	void ComputeShader::Unload_INTERNAL()
	{
		m_cs_.Reset();
		unloadDerived();
	}

	ComputeShader::ComputeShader()
		: Shader
		  (
		   "", "", SHADER_DOMAIN_OPAQUE, 0, SHADER_RASTERIZER_CULL_NONE,
		   D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT, SHADER_SAMPLER_NEVER,
		   &g_default_rtv_format, 1
		  ),
		  m_thread_{1,},
		  m_group_{1,} {}
}

namespace Engine 
{
	void* ComputePrimitiveShader::GetComputePrimitiveShader() const
	{
		return m_shader_;
	}

	void* GraphicPrimitiveShader::GetGraphicPrimitiveShader() const
	{
		return m_shader_;
	}
}