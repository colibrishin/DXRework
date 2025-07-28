#include "D3D12ComputePrimitiveShader.h"
#include <ranges>
#include <d3dcompiler.h>

#include "RenderPipeline.h"
#include "ThrowIfFailed.h"
#include "CommandPair.h"

std::vector<std::tuple<Engine::eShaderType, std::string, std::string>> Engine::D3D12ComputePrimitiveShader::s_main_version =
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
    D3D12ComputePrimitiveShader::~D3D12ComputePrimitiveShader()
    { }
    D3D12ComputePrimitiveShader::D3D12ComputePrimitiveShader()
    { }

	void D3D12ComputePrimitiveShader::Generate(Resources::ComputeShader* shader, void* pipeline_signature)
	{
		ComPtr<ID3DBlob> error;
		UINT             flag = 0;

#if SHADER_DEBUG
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
			const std::string error_message = static_cast<char*>(error->GetBufferPointer());

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

			IGraphicAPI& gi = g_graphic_accessor.GetInterface();
			const auto dev = static_cast<ID3D12Device2*>(gi.GetNativeInterface());

			DX::ThrowIfFailed
			(
				dev->CreateComputePipelineState
				(
					&desc, IID_PPV_ARGS(m_pipeline_state_.GetAddressOf())
				)
			);

			SetNativeShader(m_pipeline_state_.Get());
		}
	}
}
