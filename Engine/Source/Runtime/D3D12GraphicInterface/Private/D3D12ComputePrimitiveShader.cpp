#include "../Public/D3D12ComputePrimitiveShader.h"
#include <ranges>
#include <d3dcompiler.h>

#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"
#include "Source/Runtime/D3D12GraphicInterface/Public/ThrowIfFailed.h"
#include "source/runtime/D3D12GraphicInterface/public/CommandPair.h"

std::vector<std::tuple<Engine::eShaderType, std::string, std::string>> Engine::DX12ComputePrimitiveShader::s_main_version =
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
	DX12ComputePrimitiveShader::DX12ComputePrimitiveShader() {}

	void DX12ComputePrimitiveShader::Generate(const Weak<Resources::ComputeShader>& w_shader, void* pipeline_signature)
	{
		if (const Strong<Resources::ComputeShader>& shader = w_shader.lock())
		{
			ComPtr<ID3DBlob> error;
			UINT             flag = 0;

#if WITH_DEBUG
			flag |= D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG;
#endif

			const auto& [type, entry, version] = s_main_version[SHADER_COMPUTE];

			const auto res = D3DCompileFromFile
			(
				shader->GetPath().c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
				entry.c_str(), version.c_str(), flag, 0,
				&m_cs_, &error
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
				CD3DX12_PIPELINE_STATE_STREAM_CS cs_stream
				{
					CD3DX12_SHADER_BYTECODE(m_cs_.Get())
				};

				const D3D12_COMPUTE_PIPELINE_STATE_DESC desc
				{
					static_cast<ID3D12RootSignature*>(pipeline_signature),
					cs_stream,
					0,
					D3D12_CACHED_PIPELINE_STATE{nullptr, 0},
					D3D12_PIPELINE_STATE_FLAG_NONE
				};

				GraphicInterface& gi = g_graphic_interface.GetInterface();
				const auto dev = static_cast<ID3D12Device2*>(gi.GetNativeInterface());

				DX::ThrowIfFailed
				(
					dev->CreateComputePipelineState
					(
						&desc, IID_PPV_ARGS(m_pipeline_state_.GetAddressOf())
					)
				);
			}
		}
	}

	ID3D12PipelineState* DX12ComputePrimitiveShader::GetPSO() const
	{
		return m_pipeline_state_.Get();
	}
}
