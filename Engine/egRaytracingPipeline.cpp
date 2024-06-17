#include "pch.h"
#include "egRaytracingPipeline.hpp"

namespace Engine::Manager::Graphics
{
  void RaytracingPipeline::Initialize() {}

  void RaytracingPipeline::PreRender(const float& dt) {}

  void RaytracingPipeline::PreUpdate(const float& dt) {}

  void RaytracingPipeline::Update(const float& dt) {}

  void RaytracingPipeline::Render(const float& dt) {}

  void RaytracingPipeline::FixedUpdate(const float& dt) {}

  void RaytracingPipeline::PostRender(const float& dt) {}

  void RaytracingPipeline::PostUpdate(const float& dt) {}

  ID3D12Device5* RaytracingPipeline::GetDevice() const { return m_device_.Get(); }

  RaytracingPipeline::~RaytracingPipeline() {}

  void RaytracingPipeline::InitializeViewport()
  {
    m_viewport_ = {-1, -1, 1, 1,};
  }

  void RaytracingPipeline::InitializeInterface()
  {
    DX::ThrowIfFailed(GetD3Device().GetDevice()->QueryInterface(IID_PPV_ARGS(m_device_.GetAddressOf())));
  }

  void RaytracingPipeline::InitializeDescriptorHeaps()
  {
    m_raytracing_heap_ = GetRenderPipeline().AcquireHeapSlot().lock();
  }

  void RaytracingPipeline::InitializeRaytracingPSO()
  {
    CD3DX12_STATE_OBJECT_DESC raytracing_pipeline_desc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

    const auto& lib = raytracing_pipeline_desc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    D3D12_SHADER_BYTECODE lib_dxil = CD3DX12_SHADER_BYTECODE(); //todo: raytracing shader
    lib->SetDXILLibrary(&lib_dxil);

    lib->DefineExport(g_raytracing_gen_entrypoint);
    lib->DefineExport(g_raytracing_closest_hit_entrypoint);
    lib->DefineExport(g_raytracing_miss_entrypoint);

    const auto& hitgroup = raytracing_pipeline_desc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    hitgroup->SetClosestHitShaderImport(g_raytracing_closest_hit_entrypoint);
    hitgroup->SetHitGroupExport(g_raytracing_hitgroup_name);
    hitgroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

    const auto& shader_config = raytracing_pipeline_desc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    shader_config->Config(sizeof(Color), sizeof(Vector2));

    {
      const auto& global_root_sign = raytracing_pipeline_desc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
      global_root_sign->SetRootSignature(GetRenderPipeline().GetRootSignature());
    }

    {
      const auto& pipeline_config = raytracing_pipeline_desc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
      pipeline_config->Config(1);
    }

    DX::ThrowIfFailed(m_device_->CreateStateObject(raytracing_pipeline_desc, IID_PPV_ARGS(m_pipeline_state_.ReleaseAndGetAddressOf())));
  }

  void RaytracingPipeline::InitializeOutputBuffer()
  {
    const auto& res_desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_B8G8R8A8_UNORM, g_window_width, g_window_height, 1);
    const auto& default_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateCommittedResource
       (
        &default_heap,
        D3D12_HEAP_FLAG_NONE,
        &res_desc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        IID_PPV_ARGS(m_output_buffer_.ReleaseAndGetAddressOf())
       )
      );

    constexpr D3D12_DESCRIPTOR_HEAP_DESC heap_desc
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = 1,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
    };

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &heap_desc,
        IID_PPV_ARGS(m_output_uav_heap_.ReleaseAndGetAddressOf())
       )
      );

    constexpr D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc
    {
      .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
      .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
      .Texture2D = {0, 0}
    };

    GetD3Device().GetDevice()->CreateUnorderedAccessView
      (
       m_output_buffer_.Get(),
       nullptr,
       &uav_desc,
       m_output_uav_heap_->GetCPUDescriptorHandleForHeapStart()
      );

    m_device_->CopyDescriptorsSimple
      (
       1,
       m_raytracing_buffer_heap_->GetCPUDescriptorHandleForHeapStart(), // todo: UAV slot == 0
       m_output_uav_heap_->GetCPUDescriptorHandleForHeapStart(),
       D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
      );
  }

  void RaytracingPipeline::BuildTLAS(
    ID3D12GraphicsCommandList4*                              cmd,
    const std::map<WeakModel, std::vector<SBs::InstanceSB>>& instances
  )
  {
    const auto& default_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    VertexCollection vertices;
    IndexCollection indices;

    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instance_descs;

    UINT64 shape_idx = 0;

    for (const auto& [w_shape, instances] : instances)
    {
      if (const auto& shape = w_shape.lock())
      {
        UINT64 instance_idx = 0;

        for (const auto& mesh : shape->GetMeshes())
        {
          D3D12_RAYTRACING_INSTANCE_DESC instance_desc      = {};
          instance_desc.InstanceID                          = shape_idx;
          instance_desc.InstanceContributionToHitGroupIndex = 0;
          instance_desc.InstanceMask                        = 1;
          instance_desc.AccelerationStructure               = mesh->GetBLAS().result->GetGPUVirtualAddress();

          const auto& world_matrix = instances[instance_idx].GetParam<Matrix>(0);
          _mm256_memcpy(instance_desc.Transform, &world_matrix, sizeof(instance_desc.Transform));

          instance_descs.push_back(instance_desc);
          instance_idx++;
        }
      }

      shape_idx++;
    }

    if (m_tlas_.instanceDescSize < instance_descs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC))
    {
      m_tlas_.instanceDescSize = instance_descs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

      const auto& upload_heap         = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
      const auto& instance_descs_size = CD3DX12_RESOURCE_DESC::Buffer
        (sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instance_descs.size());

      DX::ThrowIfFailed
        (
         GetD3Device().GetDevice()->CreateCommittedResource
         (
          &upload_heap,
          D3D12_HEAP_FLAG_NONE,
          &instance_descs_size,
          D3D12_RESOURCE_STATE_GENERIC_READ,
          nullptr,
          IID_PPV_ARGS(m_tlas_.instanceDesc.ReleaseAndGetAddressOf())
         )
        );
    }

    char* data = nullptr;
    DX::ThrowIfFailed(m_tlas_.instanceDesc->Map(0, nullptr, reinterpret_cast<void**>(&data)));
    _mm256_memcpy(data, instance_descs.data(), instance_descs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
    m_tlas_.instanceDesc->Unmap(0, nullptr);

    constexpr D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS build_flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS tl_inputs;
    tl_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    tl_inputs.NumDescs = instance_descs.size();
    tl_inputs.Flags = build_flags;
    tl_inputs.InstanceDescs = m_tlas_.instanceDesc->GetGPUVirtualAddress();
    tl_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO tlas_prebuild_info = {};
    m_device_->GetRaytracingAccelerationStructurePrebuildInfo(&tl_inputs, &tlas_prebuild_info);

    if (tlas_prebuild_info.ResultDataMaxSizeInBytes == 0)
    {
      throw std::runtime_error("Top level acceleration structure prebuild info returned a size of 0.");
    }

    if (m_tlas_.resultSize < tlas_prebuild_info.ResultDataMaxSizeInBytes)
    {
      m_tlas_.resultSize = tlas_prebuild_info.ResultDataMaxSizeInBytes;
      
      const auto& res_desc     = CD3DX12_RESOURCE_DESC::Buffer
        (
         tlas_prebuild_info.ResultDataMaxSizeInBytes,
         D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
        );

      DX::ThrowIfFailed
        (
         m_device_->CreateCommittedResource
         (
          &default_heap,
          D3D12_HEAP_FLAG_NONE,
          &res_desc,
          D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
          nullptr,
          IID_PPV_ARGS(m_tlas_.result.ReleaseAndGetAddressOf())
         )
        );
    }

    if (m_tlas_.scratchSize < tlas_prebuild_info.ScratchDataSizeInBytes)
    {
      m_tlas_.scratchSize = tlas_prebuild_info.ScratchDataSizeInBytes;

      const auto& scratch_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer
        (
         tlas_prebuild_info.ScratchDataSizeInBytes,
         D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
        );

      DX::ThrowIfFailed
        (
         GetD3Device().GetDevice()->CreateCommittedResource
         (
          &default_heap,
          D3D12_HEAP_FLAG_NONE,
          &scratch_buffer_desc,
          D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
          nullptr,
          IID_PPV_ARGS(m_tlas_.scratch.GetAddressOf())
         )
        );
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build_desc = {};

    build_desc.Inputs = tl_inputs;
    build_desc.DestAccelerationStructureData = m_tlas_.result->GetGPUVirtualAddress();
    build_desc.ScratchAccelerationStructureData = m_tlas_.scratch->GetGPUVirtualAddress();

    cmd->BuildRaytracingAccelerationStructure(&build_desc, 0, nullptr);

    const auto& uav_barrier = CD3DX12_RESOURCE_BARRIER::UAV(m_tlas_.result.Get());
    cmd->ResourceBarrier(1, &uav_barrier);
  }
}
