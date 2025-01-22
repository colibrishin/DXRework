#pragma once
#include <wrl/client.h>
#include <directx/d3d12.h>
#include <directx/d3dx12.h>

#include "Source/Runtime/Resources/ComputeShader/Public/ComputeShader.h"

namespace Engine
{
	struct COMPUTEPRIMITIVESHADERDX12_API DX12ComputePrimitiveShader : public ComputePrimitiveShader
	{
		DX12ComputePrimitiveShader();
		void Generate(const Weak<Resources::ComputeShader>& w_shader, void* pipeline_signature) override;
		[[nodiscard]] ID3D12PipelineState* GetPSO() const;

	private:
		static std::vector<std::tuple<eShaderType, std::string, std::string>> s_main_version;

		ComPtr<ID3DBlob> m_cs_ = nullptr;
		ComPtr<ID3D12PipelineState> m_pipeline_state_ = nullptr;
	};
}
