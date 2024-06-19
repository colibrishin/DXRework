#include "pch.h"
#include "egRaytracingPipeline.hpp"
#include <dxcapi.h>
#include "egShape.h"

namespace Engine::Manager::Graphics
{
  void RaytracingPipeline::Initialize()
  {
    InitializeInterface();
    InitializeViewport();
    InitializeSignature();
    InitializeDescriptorHeaps();
    InitializeRaytracingPSOTMP();
    PrecompileShaders();
    InitializeOutputBuffer();
  }

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

  void RaytracingPipeline::InitializeSignature()
  {
    CD3DX12_DESCRIPTOR_RANGE1 ranges[4];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); // Output buffer
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0); // Acceleration structure, vertex buffer, index buffer, texture, normal map
    ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // Viewport buffer
    ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0); // Sampler

    CD3DX12_ROOT_PARAMETER1 root_params[4];
    root_params[0].InitAsDescriptorTable(1, &ranges[0]); // Output buffer
    root_params[1].InitAsDescriptorTable(1, &ranges[1]); // Vertex buffer
    root_params[2].InitAsConstantBufferView(0); // Viewport buffer
    root_params[3].InitAsDescriptorTable(1, &ranges[3]); // Sampler

    const auto& global_root_sign_desc = CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC
      (
       _countof(root_params),
       root_params,
       0,
       nullptr,
       D3D12_ROOT_SIGNATURE_FLAG_NONE
      );

    {
      ComPtr<ID3DBlob> signature;
      ComPtr<ID3DBlob> error;

      DX::ThrowIfFailed
        (
         D3D12SerializeVersionedRootSignature
         (
          &global_root_sign_desc,
          signature.ReleaseAndGetAddressOf(),
          error.ReleaseAndGetAddressOf()
         )
        );

      DX::ThrowIfFailed
        (
         m_device_->CreateRootSignature
         (
          0,
          signature->GetBufferPointer(),
          signature->GetBufferSize(),
          IID_PPV_ARGS(m_raytracing_global_signature_.ReleaseAndGetAddressOf())
         )
        );
    }

    CD3DX12_DESCRIPTOR_RANGE1 local_ranges[2];
    local_ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, RAYTRACING_TEX_SLOT_COUNT, 0);
    local_ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

    CD3DX12_ROOT_PARAMETER1 local_root_params[2];
    local_root_params[0].InitAsDescriptorTable(1, &local_ranges[0]); // SRV
    local_root_params[1].InitAsConstantBufferView(0); // Material CBV

    const auto& local_root_sign_desc = CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC
      (
       _countof(local_root_params),
       local_root_params,
       0,
       nullptr,
       D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE
      );

    {
      ComPtr<ID3DBlob> signature;
      ComPtr<ID3DBlob> error;

      DX::ThrowIfFailed
        (
         D3D12SerializeVersionedRootSignature
         (
          &local_root_sign_desc,
          signature.ReleaseAndGetAddressOf(),
          error.ReleaseAndGetAddressOf()
         )
        );

      DX::ThrowIfFailed
        (
         m_device_->CreateRootSignature
         (
          0,
          signature->GetBufferPointer(),
          signature->GetBufferSize(),
          IID_PPV_ARGS(m_raytracing_local_signature_.ReleaseAndGetAddressOf())
         )
        );
    }
  }

  void RaytracingPipeline::InitializeDescriptorHeaps()
  {
    constexpr D3D12_DESCRIPTOR_HEAP_DESC heap_desc
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = 7,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
    };

    DX::ThrowIfFailed
      (
       m_device_->CreateDescriptorHeap
       (
        &heap_desc,
        IID_PPV_ARGS(m_raytracing_buffer_heap_.ReleaseAndGetAddressOf())
       )
      );

    constexpr D3D12_DESCRIPTOR_HEAP_DESC sampler_heap_desc
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
      .NumDescriptors = 1,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
    };

    DX::ThrowIfFailed
      (
       m_device_->CreateDescriptorHeap
       (
        &sampler_heap_desc,
        IID_PPV_ARGS(m_raytracing_sampler_heap_.ReleaseAndGetAddressOf())
       )
      );

    m_buffer_descriptor_size_ = m_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_sampler_descriptor_size_ = m_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
  }

  void RaytracingPipeline::InitializeRaytracingPSOTMP()
  {
    // todo: move this to raytracing shader init
    // Compile shader.
    ComPtr<IDxcLibrary> library;
    DX::ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(library.ReleaseAndGetAddressOf())));

    // Compiler
    ComPtr<IDxcCompiler3> compiler;
    DX::ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(compiler.ReleaseAndGetAddressOf())));

    // Reading shader file with encoding.
    uint32_t code_page = CP_UTF8;
    ComPtr<IDxcBlobEncoding> source;
    DX::ThrowIfFailed(library->CreateBlobFromFile(L"raytracing.hlsl", &code_page, source.ReleaseAndGetAddressOf()));

    // Arguments
    ComPtr<IDxcCompilerArgs> args;
    ComPtr<IDxcUtils> utils;
    DX::ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils.ReleaseAndGetAddressOf())));
    utils->BuildArguments(L"raytracing.hlsl", L"", L"lib_6_3", nullptr, 0, nullptr, 0, args.GetAddressOf());

    // Include handler for includes.
    ComPtr<IDxcIncludeHandler> include_handler;
    DX::ThrowIfFailed(library->CreateIncludeHandler(include_handler.GetAddressOf()));

    const DxcBuffer source_buffer
    {
      .Ptr = source->GetBufferPointer(),
      .Size = source->GetBufferSize(),
      .Encoding = code_page
    };

    ComPtr<IDxcOperationResult> result;
    DX::ThrowIfFailed
      (
       compiler->Compile
       (
        &source_buffer,
        args->GetArguments(),
        args->GetCount(),
        include_handler.Get(),
        IID_PPV_ARGS(result.ReleaseAndGetAddressOf())
       )
      );

    ComPtr<IDxcBlob> blob;
    DX::ThrowIfFailed(result->GetResult(blob.GetAddressOf()));

    ComPtr<IDxcBlobEncoding> errors;
    if (SUCCEEDED(result->GetErrorBuffer(errors.ReleaseAndGetAddressOf())))
    {
      OutputDebugStringA(static_cast<const char*>(errors->GetBufferPointer()));
    }

    const auto blob_pointer = blob->GetBufferPointer();
    const auto blob_size    = blob->GetBufferSize();

    // Building pipeline state object.
    CD3DX12_STATE_OBJECT_DESC raytracing_pipeline_desc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

    // Sets the shader libraries
    const auto&                 lib      = raytracing_pipeline_desc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    const D3D12_SHADER_BYTECODE lib_dxil = 
    {
      blob->GetBufferPointer(),
      blob->GetBufferSize()
    }; //todo: raytracing shader
    lib->SetDXILLibrary(&lib_dxil);

    // Add RayGen, Miss, and Hit groups
    lib->DefineExport(g_raytracing_gen_entrypoint);
    //lib->DefineExport(g_raytracing_closest_hit_entrypoint);
    //lib->DefineExport(g_raytracing_miss_entrypoint);

    // Hit group
    //const auto& hitgroup = raytracing_pipeline_desc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    //hitgroup->SetClosestHitShaderImport(g_raytracing_closest_hit_entrypoint);
    //hitgroup->SetHitGroupExport(g_raytracing_hitgroup_name);
    //hitgroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

    // Shader payload and attribute size
    const auto& shader_config = raytracing_pipeline_desc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    shader_config->Config(sizeof(Color), sizeof(Vector2)); // uv

    // global root signature
    const auto& global_root_sign = raytracing_pipeline_desc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    global_root_sign->SetRootSignature(m_raytracing_global_signature_.Get());

    // todo: local root signature

    // Pipeline config, Recursion depth
    const auto& pipeline_config = raytracing_pipeline_desc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    pipeline_config->Config(1);

    DX::ThrowIfFailed
      (
       m_device_->CreateStateObject
       (raytracing_pipeline_desc, IID_PPV_ARGS(m_raytracing_state_object_.ReleaseAndGetAddressOf()))
      );
  }

  void RaytracingPipeline::PrecompileShaders()
  {
    // todo: raytracing shader
  }

  void RaytracingPipeline::InitializeOutputBuffer()
  {
    const auto& res_desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_B8G8R8A8_UNORM, g_window_width, g_window_height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
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
