#include "../Public/PrimitivePipelineDX12.h"
#include <directx/d3dx12.h>

namespace Engine
{
	void DX12PrimitivePipeline::Generate()
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

		DX::ThrowIfFailed(
			D3D12SerializeVersionedRootSignature
				(
				 &root_signature_desc,
				 signature_blob.GetAddressOf(),
				 error_blob.GetAddressOf()
				));

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

		GenerateHeap();

		SetPrimitivePipeline(m_root_signature_.Get());
	}

	void DX12PrimitivePipeline::GenerateHeap()
	{
		m_heap_handler_.Initialize(Managers::D3Device::GetInstance().GetDevice(), m_root_signature_.Get());
	}
}
