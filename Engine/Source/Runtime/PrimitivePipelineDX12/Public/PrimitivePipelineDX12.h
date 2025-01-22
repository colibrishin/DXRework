#pragma once
#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"

namespace Engine
{
	struct PRIMITIVEPIPELINEDX12_API DX12PrimitivePipeline : public PrimitivePipeline
	{
		void Generate() override;

		[[nodiscard]] DescriptorHandler* GetHeapHandler()
		{
			return &m_heap_handler_;
		}

	private:
		void GenerateHeap();

		ComPtr<ID3D12RootSignature> m_root_signature_;
		DescriptorHandler m_heap_handler_;
	};
}
