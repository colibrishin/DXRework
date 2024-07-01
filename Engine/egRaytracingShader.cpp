#include "pch.h"
#include "egRaytracingShader.hpp"

#include "egRaytracingPipeline.hpp"
#include "egRaytracingShaderHelper.hpp"

namespace Engine::Resources
{
  size_t RaytracingShader::GetHitGroupCount() const
  {
    return m_hit_shader_definitions_.size();
  }

  D3D12_GPU_VIRTUAL_ADDRESS RaytracingShader::GetRayGenTablePointer() const
  {
    return m_raygen_shader_table_->GetGPUVirtualAddress();
  }

  size_t RaytracingShader::GetRayGenByteSize() const
  {
    return sizeof(ShaderRecord);
  }

  D3D12_GPU_VIRTUAL_ADDRESS RaytracingShader::GetMissTablePointer() const
  {
    return m_miss_shader_table_->GetGPUVirtualAddress();
  }

  size_t RaytracingShader::GetMissByteSize() const
  {
    return sizeof(MissShaderRecord);
  }

  D3D12_GPU_VIRTUAL_ADDRESS RaytracingShader::GetHitTablePointer() const
  {
    return m_hit_shader_table_->GetGPUVirtualAddress();
  }

  size_t RaytracingShader::GetHitGroupByteSize() const
  {
    return m_hit_shader_table_used_size_;
  }

  const std::vector<RaytracingShader::RTShaderDefinition>& RaytracingShader::GetHitGroups() const
  {
    return m_hit_shader_definitions_;
  }

  void RaytracingShader::UpdateHitShaderRecords(const std::vector<HitShaderRecord>& shader_records)
  {
    if (shader_records.empty())
    {
      return;
    }

    const auto& new_size = Align(
        shader_records.size() * sizeof(HitShaderRecord), 
        D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

    // Expand the hit shader table size if the newly given data is not enough to fit in.
    if (new_size > m_hit_shader_table_size_)
    {
      const auto& default_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
      const auto& buffer_desc = CD3DX12_RESOURCE_DESC::Buffer
        (
         new_size,
         D3D12_RESOURCE_FLAG_NONE
        );

      DX::ThrowIfFailed
        (
         GetD3Device().GetDevice()->CreateCommittedResource
         (
          &default_heap,
          D3D12_HEAP_FLAG_CREATE_NOT_ZEROED, // slight performance advantage
          &buffer_desc,
          D3D12_RESOURCE_STATE_GENERIC_READ,
          nullptr,
          IID_PPV_ARGS(m_hit_shader_table_.ReleaseAndGetAddressOf())
         )
        );

      m_hit_shader_table_size_ = new_size;
    }

    char* data = nullptr;
    DX::ThrowIfFailed(m_hit_shader_table_->Map(0, nullptr, reinterpret_cast<void**>(&data)));
    _mm256_memcpy(data, shader_records.data(), shader_records.size() * sizeof(HitShaderRecord));
    m_hit_shader_table_->Unmap(0, nullptr);

    // Committed size can be larger than actually used size. 
    m_hit_shader_table_used_size_ = shader_records.size() * sizeof(HitShaderRecord);
  }

  void RaytracingShader::InitializeSampler()
  {
    constexpr D3D12_DESCRIPTOR_HEAP_DESC desc
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
      .NumDescriptors = 1,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
      .NodeMask = 0
    };

    DX::ThrowIfFailed(GetD3Device().GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_sampler_heap_.GetAddressOf())));

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

    GetD3Device().GetDevice()->CreateSampler(&sampler, m_sampler_heap_->GetCPUDescriptorHandleForHeapStart());
  }

  void RaytracingShader::InitializeLocalSignature()
  {
    // todo: custom local signature?
    // use the register space 1 for avoiding the conflict with the global root signature
    CD3DX12_DESCRIPTOR_RANGE1 hit_local_ranges;
    hit_local_ranges.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 5, 1);

    CD3DX12_ROOT_PARAMETER1 hit_local_param[6];
    hit_local_param[0].InitAsShaderResourceView(0, 1);              // light buffer todo: move to global
    hit_local_param[1].InitAsShaderResourceView(1, 1);              // material buffer
    hit_local_param[2].InitAsShaderResourceView(2, 1);              // instance buffer
    hit_local_param[3].InitAsShaderResourceView(3, 1);              // vertex buffer
    hit_local_param[4].InitAsShaderResourceView(4, 1);              // index buffer
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
       GetRaytracingPipeline().GetDevice()->CreateRootSignature
       (
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(m_local_signature_.ReleaseAndGetAddressOf())
       )
      );
  }

  void RaytracingShader::CompileShader(ComPtr<IDxcBlob>& blob) const
  {
    static ComPtr<IDxcLibrary> library;
    if (!library)
    {
      DX::ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(library.ReleaseAndGetAddressOf())));
    }

    // Compiler
    static ComPtr<IDxcCompiler3> compiler;
    if (!compiler)
    {
       DX::ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(compiler.ReleaseAndGetAddressOf())));
    }

    // Include handler for includes.
    static ComPtr<IDxcIncludeHandler> include_handler;
    if (!include_handler)
    {
      DX::ThrowIfFailed(library->CreateIncludeHandler(include_handler.GetAddressOf()));
    }

    // Reading shader file with encoding.
    uint32_t                 code_page = CP_UTF8;
    ComPtr<IDxcBlobEncoding> source;
    DX::ThrowIfFailed(library->CreateBlobFromFile(GetPath().c_str(), &code_page, source.ReleaseAndGetAddressOf()));

    // Arguments
    ComPtr<IDxcCompilerArgs> args;
    ComPtr<IDxcUtils>        utils;
    DX::ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils.ReleaseAndGetAddressOf())));
    DX::ThrowIfFailed(utils->BuildArguments(GetPath().c_str(), nullptr, L"lib_6_3", nullptr, 0, nullptr, 0, args.GetAddressOf()));

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

    DX::ThrowIfFailed(result->GetResult(blob.GetAddressOf()));

    HRESULT shader_result;
    DX::ThrowIfFailed(result->GetStatus(&shader_result));

    if (FAILED(shader_result))
    {
      ComPtr<IDxcBlobEncoding> errors;
      if (SUCCEEDED(result->GetErrorBuffer(errors.ReleaseAndGetAddressOf())))
      {
        OutputDebugStringA(static_cast<const char*>(errors->GetBufferPointer()));
      }
    }
  }

  void RaytracingShader::InitializePipelineState(const ComPtr<IDxcBlob>& blob)
  {
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
    std::vector<LPCWSTR> exports;

    if (m_export_flags_ & EXPORT_NAME_RAYGEN)
    {
      exports.push_back(g_raytracing_gen_entrypoint);
    }
    if (m_export_flags_ & EXPORT_NAME_ANY_HIT)
    {
      exports.push_back(g_raytracing_any_hit_entrypoint);
    }
    if (m_export_flags_ & EXPORT_NAME_CLOSEST_HIT)
    {
      exports.push_back(g_raytracing_closest_hit_entrypoint);
    }
    if (m_export_flags_ & EXPORT_NAME_INTERSECTION)
    {
      exports.push_back(g_raytracing_intersection_entrypoint);
    }
    if (m_export_flags_ & EXPORT_NAME_MISS)
    {
      exports.push_back(g_raytracing_miss_entrypoint);
    }
    
    lib->DefineExports(exports.data(), exports.size());

    std::vector<LPCWSTR> hit_group_names;

    for (const auto& value : m_shader_definitions_)
    {
      if (value.exportingType == EXPORT_NAME_RAYGEN || value.exportingType == EXPORT_NAME_MISS)
      {
        continue;
      }

      const auto& hitgroup = raytracing_pipeline_desc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
      if (value.exportingType == EXPORT_NAME_ANY_HIT)
      {
        hitgroup->SetAnyHitShaderImport(g_raytracing_any_hit_entrypoint);
      }
      else if (value.exportingType == EXPORT_NAME_CLOSEST_HIT)
      {
        hitgroup->SetClosestHitShaderImport(g_raytracing_closest_hit_entrypoint);
      }
      else if (value.exportingType == EXPORT_NAME_INTERSECTION)
      {
        hitgroup->SetIntersectionShaderImport(g_raytracing_intersection_entrypoint);
      }

      m_hit_shader_definitions_.push_back(value);
      hitgroup->SetHitGroupExport(value.hitGroupName.c_str());
      hitgroup->SetHitGroupType(value.hitGroupPrimitive);
      hit_group_names.push_back(value.hitGroupName.c_str());
    }

    // Shader payload and attribute size
    const auto& shader_config = raytracing_pipeline_desc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    shader_config->Config(sizeof(float[5]), sizeof(Vector2)); // barycentrics

    // global root signature
    const auto& global_root_sign = raytracing_pipeline_desc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    global_root_sign->SetRootSignature(GetRaytracingPipeline().GetGlobalSignature());

    if (!m_local_signature_)
    {
      throw std::logic_error("Local signature is not initialized");
    }

    // local root signature and hit group association
    const auto& local_root_sign = raytracing_pipeline_desc.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    local_root_sign->SetRootSignature(m_local_signature_.Get());

    const auto& local_root_export = raytracing_pipeline_desc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    local_root_export->SetSubobjectToAssociate(*local_root_sign);
    local_root_export->AddExports(hit_group_names.data(), hit_group_names.size());

    // Pipeline config, Recursion depth
    const auto& pipeline_config = raytracing_pipeline_desc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    pipeline_config->Config(m_max_recursion_);

    DX::ThrowIfFailed
      (
       GetRaytracingPipeline().GetDevice()->CreateStateObject
       (raytracing_pipeline_desc, IID_PPV_ARGS(m_pipeline_state_object_.ReleaseAndGetAddressOf()))
      );

    DX::ThrowIfFailed
      (
       m_pipeline_state_object_->QueryInterface
       (IID_PPV_ARGS(m_pso_properties_.ReleaseAndGetAddressOf()))
      );
  }

  void RaytracingShader::InitializeShaderTable()
  {
    CreateShaderTable<ShaderRecord>(
        m_pso_properties_.Get(), 
        g_raytracing_gen_entrypoint, 
        m_raygen_shader_table_.GetAddressOf());

    // todo: multiple miss shader records? dispatch description allow to do so.
    CreateShaderTable<MissShaderRecord>(
        m_pso_properties_.Get(), 
        g_raytracing_miss_entrypoint, 
        m_miss_shader_table_.GetAddressOf());

    std::vector<const wchar_t*> hitgroup_entrypoints;

    for (const auto& [type, name, primitive] : m_shader_definitions_)
    {
      std::wstring entrypoint_name;
      ID3D12Resource** resource;

      switch (type)
      {
      case EXPORT_NAME_CLOSEST_HIT:
        hitgroup_entrypoints.push_back(g_raytracing_closest_hit_entrypoint);
        break;
      case EXPORT_NAME_ANY_HIT:
        hitgroup_entrypoints.push_back(g_raytracing_any_hit_entrypoint);
        break;
      case EXPORT_NAME_INTERSECTION:
        hitgroup_entrypoints.push_back(g_raytracing_intersection_entrypoint);
        break;
      case EXPORT_NAME_RAYGEN: 
      case EXPORT_NAME_MISS:
      default: continue;
      }
    }

    // Assuming one mesh is allocated to every hit groups.
    CreateShaderTables<HitShaderRecord>(
        m_pso_properties_.Get(), 
        hitgroup_entrypoints, 
        m_hit_shader_table_.GetAddressOf());

    m_hit_shader_table_used_size_ = m_hit_shader_table_size_ = Align(
        hitgroup_entrypoints.size() * sizeof(HitShaderRecord), 
        D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
  }

  void RaytracingShader::Load_INTERNAL()
  {
    InitializeLocalSignature();

    ComPtr<IDxcBlob> blob;
    CompileShader(blob);

    const auto blob_pointer = blob->GetBufferPointer();
    const auto blob_size    = blob->GetBufferSize();

    InitializePipelineState(blob);
    InitializeSampler();
    InitializeShaderTable();
  }

  void RaytracingShader::Unload_INTERNAL()
  {
    m_sampler_heap_->Release();
    m_pipeline_state_object_->Release();
    m_pso_properties_->Release();
    m_local_signature_->Release();

    m_hit_shader_definitions_.clear();
  }
}
