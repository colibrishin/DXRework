#include "D3D12PrimitiveMesh.h"

#include <directx/d3d12.h>
#include <directx/d3dx12.h>

#include <directxtk12/BufferHelpers.h>

#include "Source/Runtime/Core/VertexElement/Public/VertexElement.h"
#include "Source/Runtime/Core/GraphicInterface.h"
#include "Source/Runtime/Core/SIMDExtension/Public/SIMDExtension.hpp"
#include "Source/Runtime/D3D12GraphicInterface/Public/ThrowIfFailed.h"
#include "Source/Runtime/D3d12Graphicinterface/Public/CommandPair.h"

namespace Engine
{
	void D3D12PrimitiveMesh::Generate(const Resources::Mesh* mesh)
    {
		std::string generic_name = mesh->GetName();

		const std::wstring vertex_name = std::wstring(generic_name.begin(), generic_name.end()) + L"VertexBuffer";

		const GraphicInterfaceContextReturnType& context = GraphicInterfaceAccessor::GetInterface().GetNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, false, L"Mesh Load Command Pair");
    	const GraphicInterfaceContextPrimitive& primitive = context.GetPointers();
    	const auto& dev = static_cast<ID3D12Device2*>(GraphicInterfaceAccessor::GetInterface().GetNativeInterface());
		const auto& cmd = static_cast<CommandPair*>(primitive.commandList);
    	
		primitive.commandList->SoftReset();
    	const VertexCollection& vertices = mesh->GetVertexCollection();
		const IndexCollection& indices = mesh->GetIndexCollection();
    	
		// -- Vertex Buffer -- //
		// Initialize vertex buffer.
		const auto& default_heap    = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		const auto& vtx_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Graphics::VertexElement) * vertices.size());

		DX::ThrowIfFailed
				(
				 dev->CreateCommittedResource
				 (
				  &default_heap,
				  D3D12_HEAP_FLAG_NONE,
				  &vtx_buffer_desc,
				  D3D12_RESOURCE_STATE_COPY_DEST,
				  nullptr,
				  IID_PPV_ARGS(m_native_vertex_buffer_.GetAddressOf())
				 )
				);

		DX::ThrowIfFailed(m_native_vertex_buffer_->SetName(vertex_name.c_str()));
    	
		// -- Upload Buffer -- //
		// Create the upload heap.
		DX::ThrowIfFailed
				(
				 DirectX::CreateUploadBuffer
				 (
				  dev,
				  vertices.data(),
				  vertices.size(),
				  m_native_vertex_upload_buffer_.GetAddressOf()
				 )
				);

		// -- Upload Data -- //
		// Copy data to the intermediate upload heap and then schedule a copy from the upload heap to the vertex buffer.
		{
			char* data = nullptr;
			DX::ThrowIfFailed(m_native_vertex_upload_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&data)));
			SIMDExtension::_mm256_memcpy(data, vertices.data(), sizeof(Graphics::VertexElement) * vertices.size());
			m_native_vertex_upload_buffer_->Unmap(0, nullptr);
		}

		cmd->GetList()->CopyResource(m_native_vertex_buffer_.Get(), m_native_vertex_upload_buffer_.Get());

		// -- Vertex Buffer View -- //
		// Initialize vertex buffer view.
		m_vertex_buffer_view_.BufferLocation = m_native_vertex_buffer_->GetGPUVirtualAddress();
		m_vertex_buffer_view_.SizeInBytes    = sizeof(Graphics::VertexElement) * static_cast<UINT>(vertices.size());
		m_vertex_buffer_view_.StrideInBytes  = sizeof(Graphics::VertexElement);

		const std::wstring index_name = std::wstring(generic_name.begin(), generic_name.end()) + L"IndexBuffer";

		const auto& idx_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT) * indices.size());

		DX::ThrowIfFailed
				(
				 dev->CreateCommittedResource
				 (
				  &default_heap,
				  D3D12_HEAP_FLAG_NONE,
				  &idx_buffer_desc,
				  D3D12_RESOURCE_STATE_COPY_DEST,
				  nullptr,
				  IID_PPV_ARGS(m_native_index_buffer_.GetAddressOf())
				 )
				);

		DX::ThrowIfFailed(m_native_index_buffer_->SetName(index_name.c_str()));
		
		// Create the upload heap.
		DX::ThrowIfFailed
				(
				 DirectX::CreateUploadBuffer
				 (
				  dev,
				  indices.data(),
				  indices.size(),
				  m_native_index_upload_buffer_.GetAddressOf()
				 )
				);

		{
			char* data = nullptr;
			DX::ThrowIfFailed(m_native_index_upload_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&data)));
			SIMDExtension::_mm256_memcpy(data, indices.data(), sizeof(UINT) * indices.size());
			m_native_index_upload_buffer_->Unmap(0, nullptr);
		}

		cmd->GetList()->CopyResource(m_native_index_buffer_.Get(), m_native_index_upload_buffer_.Get());

		m_index_buffer_view_.BufferLocation = m_native_index_buffer_->GetGPUVirtualAddress();
		m_index_buffer_view_.SizeInBytes    = sizeof(UINT) * static_cast<UINT>(indices.size());
		m_index_buffer_view_.Format         = DXGI_FORMAT_R32_UINT;

		// -- Resource Barrier -- //
		// Transition from copy dest buffer to vertex buffer.

		const auto& idx_trans = CD3DX12_RESOURCE_BARRIER::Transition
				(
				 m_native_index_buffer_.Get(),
				 D3D12_RESOURCE_STATE_COPY_DEST,
				 D3D12_RESOURCE_STATE_INDEX_BUFFER
				);
		
		const auto& vtx_trans = CD3DX12_RESOURCE_BARRIER::Transition
				(
				 m_native_vertex_buffer_.Get(),
				 D3D12_RESOURCE_STATE_COPY_DEST,
				 D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
				);

		cmd->GetList()->ResourceBarrier(1, &vtx_trans);
		cmd->GetList()->ResourceBarrier(1, &idx_trans);

		SetNativeVertexBuffer(&m_vertex_buffer_view_);
		SetNativeIndexBuffer(&m_index_buffer_view_);

#if CFG_RAYTRACING
		AccelStructBuffer& blas = GetAccelStructBuffer(mesh);
		
		if (Managers::RaytracingPipeline::GetInstance().IsRaytracingSupported() && pure_vertices.size() % 3 == 0)
		{
			// -- Structured Buffer -- //
			// structured buffer for the raytracing pipeline.
			StructuredBufferTypeInterface<Graphics::VertexElement>& sb = GetVertexStructuredBuffer(mesh);
    	
			CheckSize<UINT>(vertices.size(), L"Warning: Vertices are too many to upload!");
			sb.SetData
					(
					 &primitive,
					 static_cast<UINT>(vertices.size()),
					 vertices.data()
					);

			// Since vertices are not going to be modified, we can transition to SRV and keep it.
			sb.GetTypeless().TransitionToSRV(&primitive);
			
			const auto& vtx_pure_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Vector3) * pure_vertices.size());
			const auto& idx_pure_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT) * indices.size());

			DX::ThrowIfFailed
					(
					 dev->CreateCommittedResource
					 (
					  &default_heap,
					  D3D12_HEAP_FLAG_NONE,
					  &vtx_pure_buffer_desc,
					  D3D12_RESOURCE_STATE_COPY_DEST,
					  nullptr,
					  IID_PPV_ARGS(m_raytracing_vertex_buffer_.GetAddressOf())
					 )
					);

			DX::ThrowIfFailed
					(
					 dev->CreateCommittedResource
					 (
					  &default_heap,
					  D3D12_HEAP_FLAG_NONE,
					  &idx_pure_buffer_desc,
					  D3D12_RESOURCE_STATE_COPY_DEST,
					  nullptr,
					  IID_PPV_ARGS(m_raytracing_index_buffer_.GetAddressOf())
					 )
					);

			DX::ThrowIfFailed
					(
					 DirectX::CreateUploadBuffer
					 (
					  dev,
					  pure_vertices.data(),
					  pure_vertices.size(),
					  m_raytracing_vertex_buffer_upload_.GetAddressOf()
					 )
					);

			DX::ThrowIfFailed
					(
					 DirectX::CreateUploadBuffer
					 (
					  dev,
					  indices.data(),
					  indices.size(),
					  m_raytracing_index_buffer_upload_.GetAddressOf()
					 )
					);

			{
				char* data = nullptr;
				DX::ThrowIfFailed(m_raytracing_vertex_buffer_upload_->Map(0, nullptr, reinterpret_cast<void**>(&data)));
				SIMDExtension::_mm256_memcpy(data, pure_vertices.data(), sizeof(Vector3) * pure_vertices.size());
				m_raytracing_vertex_buffer_upload_->Unmap(0, nullptr);
			}

			{
				char* data = nullptr;
				DX::ThrowIfFailed(m_raytracing_index_buffer_upload_->Map(0, nullptr, reinterpret_cast<void**>(&data)));
				SIMDExtension::_mm256_memcpy(data, indices.data(), sizeof(UINT) * indices.size());
				m_raytracing_index_buffer_upload_->Unmap(0, nullptr);
			}

			cmd->GetList()->CopyResource(m_raytracing_vertex_buffer_.Get(), m_raytracing_vertex_buffer_upload_.Get());
			cmd->GetList()->CopyResource(m_raytracing_index_buffer_.Get(), m_raytracing_index_buffer_upload_.Get());

			const auto& vtx_pure_trans = CD3DX12_RESOURCE_BARRIER::Transition
					(
					 m_raytracing_vertex_buffer_.Get(),
					 D3D12_RESOURCE_STATE_COPY_DEST,
					 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
					);

			const auto& idx_pure_trans = CD3DX12_RESOURCE_BARRIER::Transition
					(
					 m_raytracing_index_buffer_.Get(),
					 D3D12_RESOURCE_STATE_COPY_DEST,
					 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
					);

			cmd->GetList()->ResourceBarrier(1, &idx_pure_trans);
			cmd->GetList()->ResourceBarrier(1, &vtx_pure_trans);

			// todo: animation deformation
			D3D12_RAYTRACING_GEOMETRY_DESC geo_desc{};
			geo_desc.Type      = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;

			CheckSize<uint32_t>(indices.size(), L"Warning: Index count is too large for building raytracing acceleration structure!");

			geo_desc.Triangles =
			{
				.Transform3x4 = 0,
				.IndexFormat = DXGI_FORMAT_R32_UINT,
				.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT,
				.IndexCount = static_cast<UINT>(indices.size()),
				.VertexCount = static_cast<UINT>(vertices.size()),
				.IndexBuffer = m_raytracing_index_buffer_->GetGPUVirtualAddress(),
				.VertexBuffer = {m_raytracing_vertex_buffer_->GetGPUVirtualAddress(), sizeof(Vector3)}
			};
			geo_desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS blas_inputs{};
			blas_inputs.Flags          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
			blas_inputs.Type           = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
			blas_inputs.DescsLayout    = D3D12_ELEMENTS_LAYOUT_ARRAY;
			blas_inputs.NumDescs       = 1;
			blas_inputs.pGeometryDescs = &geo_desc;

			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO blas_prebuild_info{};
			GetRaytracingPipeline().GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo
					(&blas_inputs, &blas_prebuild_info);

			if (blas_prebuild_info.ResultDataMaxSizeInBytes == 0)
			{
				throw std::runtime_error("Bottom level acceleration structure prebuild info returned a size of 0.");
			}

			const auto& result_size = Align
					(
					 blas_prebuild_info.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT
					);

			const auto& scratch_size = Align
					(blas_prebuild_info.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
			
			blas.resultPool->Update(nullptr, result_size, 1);
			blas.scratchPool->Update(nullptr, scratch_size, 1);

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC blas_desc{};

			blas_desc.DestAccelerationStructureData    = blas.resultPool->GetGPUAddress();
			blas_desc.Inputs                           = blas_inputs;
			blas_desc.ScratchAccelerationStructureData = blas.scratchPool->GetGPUAddress();

			cmd->GetList4()->BuildRaytracingAccelerationStructure
					(
					 &blas_desc,
					 0,
					 nullptr
					);

			const auto& uav_barrier = CD3DX12_RESOURCE_BARRIER::UAV(blas.resultPool->GetResource<ID3D12Resource>());
			cmd->GetList()->ResourceBarrier(1, &uav_barrier);

			blas.empty = false;
		}
		else
		{
			blas.empty = true;
		}
#endif

    	cmd->Execute();
    }
}
