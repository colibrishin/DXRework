#pragma once
#include <wrl/client.h>
#include <directx/d3d12.h>
#include <directx/d3dx12.h>

#include "Source/Runtime/Resources/ComputeShader/Public/ComputeShader.h"

namespace Engine
{
	struct ENGINE_D3D12GRAPHICINTERFACE_API D3D12ComputePrimitiveShader : public ComputePrimitiveShader
	{
		D3D12ComputePrimitiveShader();
		void Generate(Resources::ComputeShader* shader, void* pipeline_signature) override;
		
	private:
		static std::vector<std::tuple<eShaderType, std::string, std::string>> s_main_version;

		ComPtr<ID3DBlob> m_cs_ = nullptr;
		ComPtr<ID3D12PipelineState> m_pipeline_state_ = nullptr;
		Unique<StructuredBufferTypeProxy<Graphics::SBs::LocalParamSB>> m_local_param_;
	};
}
