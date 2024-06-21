#include "pch.h"
#include "egRaytracingPipeline.hpp"
#include <dxcapi.h>

#include "egManagerHelper.hpp"
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
    InitializeShaderTable();

    PrecompileShaders();
    InitializeOutputBuffer();
  }

  void RaytracingPipeline::PreRender(const float& dt)
  {
    GetD3Device().ClearRenderTarget();
  }

  void RaytracingPipeline::PreUpdate(const float& dt) {}

  void RaytracingPipeline::Update(const float& dt) {}

  void RaytracingPipeline::Render(const float& dt)
  {
    if (g_raytracing)
    {
      const auto& cmd = GetD3Device().AcquireCommandPair(L"Raytracing Rendering").lock();

      cmd->SoftReset();

      for (const auto& buffer : m_used_buffers_)
      {
        buffer->TransitionToSRV(cmd->GetList());
      }

      for (const auto& tex : m_used_textures_)
      {
        tex->ManualTransition(cmd->GetList(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
      }

      GetRayTracer().GetLightSB().TransitionToSRV(cmd->GetList());

      DefaultRootSignature(cmd->GetList4());
      cmd->GetList4()->SetPipelineState1(m_raytracing_state_object_.Get());
      cmd->GetList4()->RSSetViewports(1, &m_viewport_);
      cmd->GetList4()->RSSetScissorRects(1, &m_scissor_rect_);

      DefaultDescriptorHeap(cmd->GetList4());
      m_wvp_buffer_.Bind(cmd, {});
      cmd->GetList()->SetComputeRootConstantBufferView(4, m_wvp_buffer_.GetGPUAddress());
      BindTLAS(cmd->GetList4());
      
      const D3D12_DISPATCH_RAYS_DESC dispatch_desc
      {
        .RayGenerationShaderRecord = {
          m_raygen_shader_table_->GetGPUVirtualAddress(),
          sizeof(ShaderRecord)
        },
        .MissShaderTable = {
          .StartAddress = m_miss_shader_table_->GetGPUVirtualAddress(),
          .SizeInBytes = sizeof(ShaderRecord),
          .StrideInBytes = sizeof(ShaderRecord)
        },
        .HitGroupTable = {
          .StartAddress = m_closest_hit_shader_table_->GetGPUVirtualAddress(),
          .SizeInBytes = m_closest_hit_shader_table_size_,
          .StrideInBytes = sizeof(HitShaderRecord)
        },
        .CallableShaderTable = {},
        .Width = g_window_width,
        .Height = g_window_height,
        .Depth = 1
      };

      cmd->GetList4()->DispatchRays(&dispatch_desc);

      const auto& copy_barrier = CD3DX12_RESOURCE_BARRIER::Transition
        (
         m_output_buffer_.Get(),
         D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
         D3D12_RESOURCE_STATE_COPY_SOURCE
        );

      const auto& dst_barrier = CD3DX12_RESOURCE_BARRIER::Transition
        (
         GetD3Device().GetRenderTarget(GetD3Device().GetFrameIndex()),
         D3D12_RESOURCE_STATE_RENDER_TARGET,
         D3D12_RESOURCE_STATE_COPY_DEST
        );

      cmd->GetList4()->ResourceBarrier(1, &copy_barrier);
      cmd->GetList4()->ResourceBarrier(1, &dst_barrier);

      cmd->GetList4()->CopyResource
        (
         GetD3Device().GetRenderTarget(GetD3Device().GetFrameIndex()),
         m_output_buffer_.Get()
        );

      const auto& uav_barrier = CD3DX12_RESOURCE_BARRIER::Transition
        (
         m_output_buffer_.Get(),
         D3D12_RESOURCE_STATE_COPY_SOURCE,
         D3D12_RESOURCE_STATE_UNORDERED_ACCESS
        );

      const auto& rtv_barrier = CD3DX12_RESOURCE_BARRIER::Transition
        (
         GetD3Device().GetRenderTarget(GetD3Device().GetFrameIndex()),
         D3D12_RESOURCE_STATE_COPY_DEST,
         D3D12_RESOURCE_STATE_RENDER_TARGET
        );

      cmd->GetList4()->ResourceBarrier(1, &uav_barrier);
      cmd->GetList4()->ResourceBarrier(1, &rtv_barrier);

      for (const auto& buffer : m_used_buffers_)
      {
        buffer->TransitionCommon(cmd->GetList(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
      }

      for (const auto& tex : m_used_textures_)
      {
        tex->ManualTransition(cmd->GetList(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
      }

      GetRayTracer().GetLightSB().TransitionCommon(cmd->GetList(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

      cmd->FlagReady();

      m_used_buffers_.clear();
      m_used_textures_.clear();
      m_tmp_textures_heaps_.clear(); // reuse it
    }
  }

  void RaytracingPipeline::FixedUpdate(const float& dt) {}

  void RaytracingPipeline::PostRender(const float& dt) {}

  void RaytracingPipeline::PostUpdate(const float& dt) {}

  ID3D12Device5* RaytracingPipeline::GetDevice() const { return m_device_.Get(); }

  RaytracingPipeline::~RaytracingPipeline() {}

  void RaytracingPipeline::InitializeViewport()
  {
    m_viewport_ = 
    {
      .TopLeftX = 0.0f,
      .TopLeftY = 0.0f,
      .Width = static_cast<float>(g_window_width),
      .Height = static_cast<float>(g_window_height),
      .MinDepth = 0.0f,
      .MaxDepth = 1.0f
    };

    m_scissor_rect_ =
    {
      .left = 0,
      .top = 0,
      .right = static_cast<LONG>(g_window_width),
      .bottom = static_cast<LONG>(g_window_height)
    };
  }

  void RaytracingPipeline::InitializeInterface()
  {
    DX::ThrowIfFailed(GetD3Device().GetDevice()->QueryInterface(IID_PPV_ARGS(m_device_.GetAddressOf())));
  }

  void RaytracingPipeline::InitializeSignature()
  {
    CD3DX12_DESCRIPTOR_RANGE1 ranges[4];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); // Output buffer
    //ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1); // vertex buffer, index buffer
    //ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // WVP buffer
    ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0); // Sampler

    CD3DX12_ROOT_PARAMETER1 root_params[6];
    root_params[0].InitAsDescriptorTable(1, &ranges[0]); // Output buffer
    root_params[1].InitAsShaderResourceView(0); // Acceleration structure
    root_params[2].InitAsShaderResourceView(1); // vertex buffer
    root_params[3].InitAsShaderResourceView(2); // index buffer
    root_params[4].InitAsConstantBufferView(0); // WVP buffer
    root_params[5].InitAsDescriptorTable(1, &ranges[3]); // Sampler

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

      if (error)
      {
        OutputDebugStringA(static_cast<const char*>(error->GetBufferPointer()));
      }

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

    {
      // use the register space 1 for avoiding the conflict with the global root signature
      CD3DX12_DESCRIPTOR_RANGE1 hit_local_ranges;
      hit_local_ranges.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 5, 1);

      CD3DX12_ROOT_PARAMETER1 hit_local_param[6];
      hit_local_param[0].InitAsShaderResourceView(0, 1); // light buffer
      hit_local_param[1].InitAsShaderResourceView(1, 1); // material buffer
      hit_local_param[2].InitAsShaderResourceView(2, 1); // instance buffer
      hit_local_param[3].InitAsShaderResourceView(3, 1); // vertex buffer
      hit_local_param[4].InitAsShaderResourceView(4, 1); // index buffer
      hit_local_param[5].InitAsDescriptorTable(1, &hit_local_ranges); // texture, normal

      const auto& local_root_sign_desc = CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC
        (
         _countof(hit_local_param),
         hit_local_param,
         0,
         nullptr,
         D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE
        );


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
          IID_PPV_ARGS(m_raytracing_hit_local_signature_.ReleaseAndGetAddressOf())
         )
        );
    }
  }

  void RaytracingPipeline::InitializeDescriptorHeaps()
  {
    constexpr D3D12_DESCRIPTOR_HEAP_DESC heap_desc
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = 2,
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

    m_wvp_buffer_.Create(nullptr);
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
    utils->BuildArguments(L"raytracing.hlsl", nullptr, L"lib_6_3", nullptr, 0, nullptr, 0, args.GetAddressOf());

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
    };

    lib->SetDXILLibrary(&lib_dxil);

    // Add RayGen, Miss, and Hit groups
    const wchar_t* export_names[] = 
    {
      g_raytracing_gen_entrypoint,
      g_raytracing_closest_hit_entrypoint,
      g_raytracing_miss_entrypoint
    };
    lib->DefineExports(export_names);

    // Hit group
    const auto& hitgroup = raytracing_pipeline_desc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    hitgroup->SetClosestHitShaderImport(g_raytracing_closest_hit_entrypoint);
    hitgroup->SetHitGroupExport(g_raytracing_hitgroup_name);
    hitgroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

    // Shader payload and attribute size
    const auto& shader_config = raytracing_pipeline_desc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    shader_config->Config(sizeof(Color), sizeof(Vector2)); // barycentrics

    // global root signature
    const auto& global_root_sign = raytracing_pipeline_desc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    global_root_sign->SetRootSignature(m_raytracing_global_signature_.Get());

    const auto& local_root_sign = raytracing_pipeline_desc.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    local_root_sign->SetRootSignature(m_raytracing_hit_local_signature_.Get());

    const auto& local_root_export = raytracing_pipeline_desc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    local_root_export->SetSubobjectToAssociate(*local_root_sign);
    local_root_export->AddExport(g_raytracing_hitgroup_name);

    // Pipeline config, Recursion depth
    const auto& pipeline_config = raytracing_pipeline_desc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    pipeline_config->Config(1);

    DX::ThrowIfFailed
      (
       m_device_->CreateStateObject
       (raytracing_pipeline_desc, IID_PPV_ARGS(m_raytracing_state_object_.ReleaseAndGetAddressOf()))
      );

    DX::ThrowIfFailed
      (
       m_raytracing_state_object_->QueryInterface
       (IID_PPV_ARGS(m_raytracing_state_object_properties_.ReleaseAndGetAddressOf()))
      );

    constexpr D3D12_SAMPLER_DESC sampler
    {
      .Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
      .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
      .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
      .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
      .MipLODBias = 0,
      .MaxAnisotropy = 0,
      .ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
      .BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
      .MinLOD = 0,
      .MaxLOD = D3D12_FLOAT32_MAX
    };

    m_device_->CreateSampler(&sampler, m_raytracing_sampler_heap_->GetCPUDescriptorHandleForHeapStart());
  }

  void RaytracingPipeline::InitializeShaderTable()
  {
    ShaderRecord empty_record{};

    const auto& upload_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    const auto& buffer_desc = CD3DX12_RESOURCE_DESC::Buffer
      (
       Align(sizeof(ShaderRecord), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT),
       D3D12_RESOURCE_FLAG_NONE
      );

    {
      _mm256_memcpy
       (
        empty_record.shaderId, m_raytracing_state_object_properties_->GetShaderIdentifier
        (g_raytracing_gen_entrypoint), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
       );

      DX::ThrowIfFailed
        (
         m_device_->CreateCommittedResource
         (
          &upload_heap,
          D3D12_HEAP_FLAG_NONE,
          &buffer_desc,
          D3D12_RESOURCE_STATE_GENERIC_READ,
          nullptr,
          IID_PPV_ARGS(m_raygen_shader_table_.ReleaseAndGetAddressOf())
         )
        );

      char* data = nullptr;
      DX::ThrowIfFailed(m_raygen_shader_table_->Map(0, nullptr, reinterpret_cast<void**>(&data)));
      _mm256_memcpy(data, &empty_record, sizeof(ShaderRecord));
      m_raygen_shader_table_->Unmap(0, nullptr);
    }

    {
      HitShaderRecord empty_hit_record{};

      const auto& hit_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer
      (
       Align(sizeof(HitShaderRecord), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT),
       D3D12_RESOURCE_FLAG_NONE
      );

      _mm256_memcpy
        (
         empty_hit_record.shaderId, m_raytracing_state_object_properties_->GetShaderIdentifier
         (g_raytracing_hitgroup_name), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
        );

      DX::ThrowIfFailed
        (
         m_device_->CreateCommittedResource
         (
          &upload_heap,
          D3D12_HEAP_FLAG_NONE,
          &hit_buffer_desc,
          D3D12_RESOURCE_STATE_GENERIC_READ,
          nullptr,
          IID_PPV_ARGS(m_closest_hit_shader_table_.ReleaseAndGetAddressOf())
         )
        );

      char* data = nullptr;
      DX::ThrowIfFailed(m_closest_hit_shader_table_->Map(0, nullptr, reinterpret_cast<void**>(&data)));
      _mm256_memcpy(data, &empty_hit_record, sizeof(HitShaderRecord));
      m_closest_hit_shader_table_->Unmap(0, nullptr);

      m_closest_hit_shader_table_size_ = Align(sizeof(HitShaderRecord), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    }

    {
      _mm256_memcpy
        (
         empty_record.shaderId, m_raytracing_state_object_properties_->GetShaderIdentifier
         (g_raytracing_miss_entrypoint), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
        );

      DX::ThrowIfFailed
        (
         m_device_->CreateCommittedResource
         (
          &upload_heap,
          D3D12_HEAP_FLAG_NONE,
          &buffer_desc,
          D3D12_RESOURCE_STATE_GENERIC_READ,
          nullptr,
          IID_PPV_ARGS(m_miss_shader_table_.ReleaseAndGetAddressOf())
         )
        );

      char* data = nullptr;
      DX::ThrowIfFailed(m_miss_shader_table_->Map(0, nullptr, reinterpret_cast<void**>(&data)));
      _mm256_memcpy(data, &empty_record, sizeof(ShaderRecord));
      m_miss_shader_table_->Unmap(0, nullptr);
    }
  }

  void RaytracingPipeline::PrecompileShaders()
  {
    // todo: raytracing shader
  }

  void RaytracingPipeline::InitializeOutputBuffer()
  {
    const auto& res_desc = CD3DX12_RESOURCE_DESC::Tex2D
      (
       DXGI_FORMAT_R8G8B8A8_UNORM, g_window_width, g_window_height, 1, 1, 1, 0,
       D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
      );
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
      .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
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

  void RaytracingPipeline::UpdateHitShaderRecords()
  {
    if (m_hit_shader_records_.empty())
    {
      return;
    }

    const auto& new_size = Align(m_hit_shader_records_.size() * sizeof(HitShaderRecord), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

    if (new_size > m_closest_hit_shader_table_size_)
    {
      const auto& default_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
      const auto& buffer_desc = CD3DX12_RESOURCE_DESC::Buffer
        (
         new_size,
         D3D12_RESOURCE_FLAG_NONE
        );

      DX::ThrowIfFailed
        (
         m_device_->CreateCommittedResource
         (
          &default_heap,
          D3D12_HEAP_FLAG_NONE,
          &buffer_desc,
          D3D12_RESOURCE_STATE_GENERIC_READ,
          nullptr,
          IID_PPV_ARGS(m_closest_hit_shader_table_.ReleaseAndGetAddressOf())
         )
        );

      m_closest_hit_shader_table_size_ = new_size;
    }

    char* data = nullptr;
    DX::ThrowIfFailed(m_closest_hit_shader_table_->Map(0, nullptr, reinterpret_cast<void**>(&data)));
    _mm256_memcpy(data, m_hit_shader_records_.data(), m_hit_shader_records_.size() * sizeof(HitShaderRecord));
    m_closest_hit_shader_table_->Unmap(0, nullptr);

    m_hit_shader_records_.clear();
  }

  void RaytracingPipeline::BuildTLAS(
    ID3D12GraphicsCommandList4*                                 cmd,
    const std::map<WeakMaterial, std::vector<SBs::InstanceSB>>& instances,
    std::vector<Graphics::StructuredBuffer<SBs::InstanceSB>>&   instance_sb
  )
  {
    const auto& default_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    std::vector<D3D12_RAYTRACING_INSTANCE_DESC>             instance_descs;
    std::map<std::pair<StrongMesh, StrongMaterial>, UINT64> mesh_map;
    
    UINT64 mesh_idx = 0;

    for (const auto& [w_mtr, instances] : instances)
    {
      if (const StrongMaterial& mtr = w_mtr.lock())
      {
        if (const StrongModel& shape = mtr->GetResource<Resources::Shape>(0).lock())
        {
          mtr->UpdateMaterialSB(cmd);
          Graphics::StructuredBuffer<Graphics::SBs::MaterialSB>& material_sb = mtr->GetMaterialSBBuffer();

          m_used_buffers_.push_back(&material_sb);

          constexpr D3D12_DESCRIPTOR_HEAP_DESC heap_desc
          {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            .NumDescriptors = 2,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
          };

          DX::ThrowIfFailed
            (
             GetD3Device().GetDevice()->CreateDescriptorHeap
             (
              &heap_desc,
              IID_PPV_ARGS(m_tmp_textures_heaps_.emplace_back().ReleaseAndGetAddressOf())
             )
            );

          if (const StrongTexture& tex = mtr->GetResource<Resources::Texture>(0).lock())
          {
            m_device_->CopyDescriptorsSimple
              (
               1,
               m_tmp_textures_heaps_.back()->GetCPUDescriptorHandleForHeapStart(),
               tex->GetSRVDescriptor()->GetCPUDescriptorHandleForHeapStart(),
               D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
              );

            m_used_textures_.insert(tex);
          }

          if (const StrongTexture& norm = mtr->GetResource<Resources::Texture>(1).lock())
          {
            CD3DX12_CPU_DESCRIPTOR_HANDLE handle
              (
               m_tmp_textures_heaps_.back()->GetCPUDescriptorHandleForHeapStart(),
               1,
               m_buffer_descriptor_size_
              );

            m_device_->CopyDescriptorsSimple
              (
               1,
               handle,
               norm->GetSRVDescriptor()->GetCPUDescriptorHandleForHeapStart(),
               D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
              );

            m_used_textures_.insert(norm);
          }

          for (const StrongMesh& mesh : shape->GetMeshes())
          {
            if (mesh->GetBLAS().empty)
            {
              continue; // Billboards and other non-triangle meshes
            }

            mesh_map[{mesh, mtr}] = mesh_idx;

            instance_sb[mesh_idx].SetData(cmd, instances.size(), instances.data());

            m_used_buffers_.push_back(&instance_sb[mesh_idx]);

            // Add hit group per mesh
            HitShaderRecord record
            {
              .lightSB = GetRayTracer().GetLightSB().GetGPUAddress(),
              .materialSB = material_sb.GetGPUAddress(),
              .instanceSB = instance_sb[mesh_idx].GetGPUAddress(),
              .vertices = mesh->GetVertexStructuredBuffer().GetGPUAddress(),
              .indices = mesh->GetIndexBuffer()->GetGPUVirtualAddress(),
              .textures = m_tmp_textures_heaps_.back()->GetGPUDescriptorHandleForHeapStart(),
            };

            _mm256_memcpy
              (
               record.shaderId, m_raytracing_state_object_properties_->GetShaderIdentifier
               (g_raytracing_hitgroup_name), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
              );

            m_hit_shader_records_.push_back(record);
            mesh_idx++;
          }

          for (int i = 0; i < instances.size(); ++i)
          {
            for (const StrongMesh& mesh : shape->GetMeshes())
            {
              if (mesh->GetBLAS().empty)
              {
                continue; // Billboards and other non-triangle meshes
              }

              D3D12_RAYTRACING_INSTANCE_DESC instance_desc      = {};
              instance_desc.InstanceID                          = i;
              instance_desc.InstanceContributionToHitGroupIndex = mesh_map[{mesh, mtr}];
              instance_desc.InstanceMask                        = 1;
              instance_desc.AccelerationStructure               = mesh->GetBLAS().result->GetGPUVirtualAddress();
              instance_desc.Flags                               = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

              // todo: animation deformation should be applied at some point.
              const auto& world_matrix = instances[i].GetParam<Matrix>(0);
              _mm256_memcpy(instance_desc.Transform, &world_matrix, sizeof(instance_desc.Transform));

              instance_descs.push_back(instance_desc);
            }
          }
        }
      }
    }

    const auto& instance_descs_size = Align(instance_descs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC), D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT);

    if (m_tlas_.instanceDescSize < instance_descs_size)
    {
      m_tlas_.instanceDescSize = instance_descs_size;

      const auto& upload_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
      const auto& buffer_desc = CD3DX12_RESOURCE_DESC::Buffer
        (instance_descs_size);

      DX::ThrowIfFailed
        (
         GetD3Device().GetDevice()->CreateCommittedResource
         (
          &upload_heap,
          D3D12_HEAP_FLAG_NONE,
          &buffer_desc,
          D3D12_RESOURCE_STATE_GENERIC_READ,
          nullptr,
          IID_PPV_ARGS(m_tlas_.instanceDesc.ReleaseAndGetAddressOf())
         )
        );
    }

    if (m_tlas_.instanceDescSize > 0)
    {
      char* data = nullptr;
      DX::ThrowIfFailed(m_tlas_.instanceDesc->Map(0, nullptr, reinterpret_cast<void**>(&data)));
      _mm256_memcpy(data, instance_descs.data(), instance_descs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
      m_tlas_.instanceDesc->Unmap(0, nullptr);
    }

    constexpr D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS build_flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS tl_inputs{};
    tl_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    tl_inputs.NumDescs = instance_descs.size();
    tl_inputs.Flags = build_flags;
    tl_inputs.InstanceDescs = m_tlas_.instanceDesc ? m_tlas_.instanceDesc->GetGPUVirtualAddress() : 0;
    tl_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO tlas_prebuild_info = {};
    m_device_->GetRaytracingAccelerationStructurePrebuildInfo(&tl_inputs, &tlas_prebuild_info);

    if (tlas_prebuild_info.ResultDataMaxSizeInBytes == 0)
    {
      throw std::runtime_error("Top level acceleration structure prebuild info returned a size of 0.");
    }

    const auto& result_size = Align(tlas_prebuild_info.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
    const auto& scratch_size = Align(tlas_prebuild_info.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);

    if (m_tlas_.resultSize < result_size)
    {
      m_tlas_.resultSize = result_size;
      
      const auto& res_desc     = CD3DX12_RESOURCE_DESC::Buffer
        (
         result_size,
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

    if (m_tlas_.scratchSize < scratch_size)
    {
      m_tlas_.scratchSize = scratch_size;

      const auto& scratch_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer
        (
         scratch_size,
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

    UpdateHitShaderRecords();

    m_tlas_.empty = false;
  }

  void RaytracingPipeline::SetPerspectiveMatrix(const CBs::PerspectiveCB& matrix)
  {
    m_wvp_buffer_.SetData(&matrix);
  }

  void RaytracingPipeline::DefaultRootSignature(ID3D12GraphicsCommandList1* cmd) const
  {
    cmd->SetComputeRootSignature(m_raytracing_global_signature_.Get());
  }

  void RaytracingPipeline::DefaultDescriptorHeap(ID3D12GraphicsCommandList1* cmd) const
  {
    ID3D12DescriptorHeap* heaps[] = 
    {
      m_raytracing_buffer_heap_.Get(),
      m_raytracing_sampler_heap_.Get()
    };

    cmd->SetDescriptorHeaps(2, heaps);

    cmd->SetComputeRootDescriptorTable(0, m_raytracing_buffer_heap_->GetGPUDescriptorHandleForHeapStart());
    cmd->SetComputeRootDescriptorTable(5, m_raytracing_sampler_heap_->GetGPUDescriptorHandleForHeapStart());
  }

  void RaytracingPipeline::BindTLAS(ID3D12GraphicsCommandList1* cmd) const
  {
    cmd->SetComputeRootShaderResourceView(1, m_tlas_.result->GetGPUVirtualAddress());
  }
}
