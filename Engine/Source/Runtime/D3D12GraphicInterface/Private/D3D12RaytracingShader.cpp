#if CFG_RAYTRACING
#include "D3D12RaytracingShader.h"
#include <directx-dxc/dxcapi.h>
#include <directx-dxc/d3d12shader.h>

#include "SIMDExtension.hpp"
#include "RaytracingShader.h"
#include "ThrowIfFailed.h"

namespace Engine
{
    void D3D12RaytracingShader::Generate(const Resources::RaytracingShader* shader, void* pipeline_signature)
    {
        IRaytracingExtension& rgi = s_ga.GetRaytracingInterface();
        const auto& dev = static_cast<ID3D12Device5*>(rgi.GetRaytracingNativeInterface());
        const auto& raytracing_root_pipeline = static_cast<ID3D12RootSignature*>(pipeline_signature);

        InitializeLocalSignature(dev);
        
		// Compile shader.
		ComPtr<IDxcLibrary> library;
		DX::ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(library.ReleaseAndGetAddressOf())));

		// Compiler
		ComPtr<IDxcCompiler3> compiler;
		DX::ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(compiler.ReleaseAndGetAddressOf())));

		// Reading shader file with encoding.
		uint32_t                 code_page = CP_UTF8;
		ComPtr<IDxcBlobEncoding> source;
		DX::ThrowIfFailed(library->CreateBlobFromFile(shader->GetPath().c_str(), &code_page, source.ReleaseAndGetAddressOf()));

		// Arguments
		ComPtr<IDxcCompilerArgs> args;
		ComPtr<IDxcUtils>        utils;
		DX::ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils.ReleaseAndGetAddressOf())));
        
		DX::ThrowIfFailed(utils->BuildArguments(
		    shader->GetPath().c_str(),
		    nullptr,
		    L"lib_6_3",
		    nullptr,
		    0,
		    nullptr,
		    0,
		    args.GetAddressOf()));

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

		// Building pipeline state object.
		CD3DX12_STATE_OBJECT_DESC raytracing_pipeline_desc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

		// Sets the shader libraries
		const auto&                 lib = raytracing_pipeline_desc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
		const D3D12_SHADER_BYTECODE lib_dxil =
		{
			blob->GetBufferPointer(),
			blob->GetBufferSize()
		};

		lib->SetDXILLibrary(&lib_dxil);

		// Add RayGen, Miss, and Hit groups
        std::vector<const wchar_t*> exporting_names;
        const std::array<bool, 4>& to_export = shader->GetHasExport();
        
        for (size_t i = 0; i < to_export.size(); ++i)
        {
            if (to_export[i])
            {
                exporting_names.push_back(g_raytracing_export_names[i]);
            }
        }
        
		lib->DefineExports(exporting_names.data(), exporting_names.size());
        
        if (to_export[RAY_SHADER_CLOSEST_HIT])
        {
            // Hit group
            const auto& hitgroup = raytracing_pipeline_desc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroup->SetClosestHitShaderImport(g_raytracing_export_names[RAY_SHADER_CLOSEST_HIT]);
            if (to_export[RAY_SHADER_ANY_HIT])
            {
                hitgroup->SetAnyHitShaderImport(g_raytracing_export_names[RAY_SHADER_ANY_HIT]);
            }
            hitgroup->SetHitGroupExport(shader->GetHitGroupName().data());
            hitgroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
        }

		// Shader payload and attribute size
		const auto& shader_config = raytracing_pipeline_desc.CreateSubobject<
			CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
		shader_config->Config(sizeof(float[6]), sizeof(Vector2)); // barycentrics

		// global root signature
		const auto& global_root_sign = raytracing_pipeline_desc.CreateSubobject<
			CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
		global_root_sign->SetRootSignature(raytracing_root_pipeline);

        // local root signature
		const auto& local_root_sign = raytracing_pipeline_desc.CreateSubobject<
			CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
		local_root_sign->SetRootSignature(m_local_root_signature_.Get());

		const auto& local_root_export = raytracing_pipeline_desc.CreateSubobject<
			CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
		local_root_export->SetSubobjectToAssociate(*local_root_sign);

        if (to_export[RAY_SHADER_ANY_HIT] || to_export[RAY_SHADER_CLOSEST_HIT])
        {
            local_root_export->AddExport(shader->GetHitGroupName().data());
        }

        SetExports( shader->GetHasExport() );

		// Pipeline config, Recursion depth
		const auto& pipeline_config = raytracing_pipeline_desc.CreateSubobject<
			CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
		pipeline_config->Config(1 + (2 * CFG_MAX_DIRECTIONAL_LIGHT)); // Default recursion + shadow rays (light counts)

		DX::ThrowIfFailed(
				 dev->CreateStateObject(
				     raytracing_pipeline_desc,
				     IID_PPV_ARGS(m_raytracing_pso_.ReleaseAndGetAddressOf() ) ) );

		DX::ThrowIfFailed(
				 m_raytracing_pso_->QueryInterface(
				     IID_PPV_ARGS(m_raytracing_pso_properties_.ReleaseAndGetAddressOf())));

		const D3D12_SAMPLER_DESC sampler
		{
			.Filter = static_cast<D3D12_FILTER>(shader->GetSamplerFilter()),
			.AddressU = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(shader->GetSamplerAddressMode()),
			.AddressV = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(shader->GetSamplerAddressMode()),
			.AddressW = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(shader->GetSamplerAddressMode()),
			.MipLODBias = 0,
			.MaxAnisotropy = 0,
			.ComparisonFunc = static_cast<D3D12_COMPARISON_FUNC>(shader->GetSamplerFunction()),
			.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
			.MinLOD = 0,
			.MaxLOD = D3D12_FLOAT32_MAX
		};

        constexpr D3D12_DESCRIPTOR_HEAP_DESC desc
        {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
            .NumDescriptors = 1,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            .NodeMask = 0
        };
    
        dev->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_sampler_heap_.GetAddressOf()));
		dev->CreateSampler(&sampler, m_sampler_heap_->GetCPUDescriptorHandleForHeapStart());

        InitializeShaderTable(shader, dev);
        
        SetNativeShader(m_raytracing_pso_.Get());
        SetNativeSampler(m_sampler_heap_.Get());
    }

    void* D3D12RaytracingShader::GetShaderRecord(const size_t idx) const
    {
        if (idx >= std::size(m_shader_tables_))
        {
            return nullptr;
        }
        
        return m_shader_tables_[idx].Get();
    }

    void D3D12RaytracingShader::UpdateShaderRecords(const eRaytracingShaderRecordType type, const byte_stream& records)
    {
        IGraphicAPI& gi = s_ga.GetInterface();
        auto* dev = static_cast<ID3D12Device2*>(gi.GetNativeInterface());
        
        auto& hit_record = m_shader_tables_[type];
        const auto& new_size = Align(
            records.size(),
            D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
        
        if (new_size > m_allocated_shader_record_size_[type])
        {
            const auto& default_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            const auto& buffer_desc  = CD3DX12_RESOURCE_DESC::Buffer
                    (
                     new_size,
                     D3D12_RESOURCE_FLAG_NONE
                    );

            DX::ThrowIfFailed(
                     dev->CreateCommittedResource
                     (
                      &default_heap,
                      D3D12_HEAP_FLAG_NONE,
                      &buffer_desc,
                      D3D12_RESOURCE_STATE_GENERIC_READ,
                      nullptr,
                      IID_PPV_ARGS(hit_record.ReleaseAndGetAddressOf())
                     ));

            m_allocated_shader_record_size_[type] = new_size;
        }

        const wchar_t* key;
        switch (type)
        {
        case RAY_SHADER_REC_GEN:
            key = g_raytracing_export_names[RAY_SHADER_GEN];
            break;
        case RAY_SHADER_REC_MISS:
            key = g_raytracing_export_names[RAY_SHADER_MISS];
            break;
        case RAY_SHADER_REC_HIT:
            key = m_hit_group_name_.data();
            break;
        default:
            throw std::runtime_error("Unknown type iterated");
        }
        
        char* copy_dst = nullptr;
        DX::ThrowIfFailed(hit_record->Map(0, nullptr, reinterpret_cast<void**>(&copy_dst)));
        SIMDExtension::_mm256_memcpy(copy_dst, records.data(), records.size());
        std::memcpy(copy_dst, m_raytracing_pso_properties_->GetShaderIdentifier(key), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        hit_record->Unmap(0, nullptr);
    }

    void D3D12RaytracingShader::InitializeLocalSignature(ID3D12Device5* dev)
    {
        CD3DX12_DESCRIPTOR_RANGE1 ranges[RAYTRACING_LOCAL_SLOT_RANGE_END];
        ranges[RAYTRACING_LOCAL_SLOT_SRV].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, g_max_engine_texture_slots, 0);
        ranges[RAYTRACING_LOCAL_SLOT_UAV].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, g_max_uav_slots, 0);
        ranges[RAYTRACING_LOCAL_SLOT_SAMPLER].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, SAMPLER_END, 0);

        ranges[RAYTRACING_LOCAL_SLOT_SRV].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
        ranges[RAYTRACING_LOCAL_SLOT_UAV].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
        ranges[RAYTRACING_LOCAL_SLOT_SAMPLER].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

        CD3DX12_ROOT_PARAMETER1 root_parameters[RAYTRACING_LOCAL_SLOT_END];
        root_parameters[RAYTRACING_LOCAL_SLOT_SRV].InitAsDescriptorTable
        (1, &ranges[RAYTRACING_LOCAL_SLOT_SRV], D3D12_SHADER_VISIBILITY_ALL);
        root_parameters[RAYTRACING_LOCAL_SLOT_UAV].InitAsDescriptorTable
        (1, &ranges[RAYTRACING_LOCAL_SLOT_UAV], D3D12_SHADER_VISIBILITY_ALL);
        root_parameters[RAYTRACING_LOCAL_SLOT_SAMPLER].InitAsDescriptorTable
        (1, &ranges[RAYTRACING_LOCAL_SLOT_SAMPLER], D3D12_SHADER_VISIBILITY_ALL);
        root_parameters[RAYTRACING_LOCAL_SLOT_VERTEX].InitAsShaderResourceView( 3, 1 );
        root_parameters[RAYTRACING_LOCAL_SLOT_INDEX].InitAsShaderResourceView( 4, 1 );
        
        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;

        const CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc(
            RAYTRACING_LOCAL_SLOT_END,
            root_parameters,
            0,
            nullptr,
            D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

        DX::ThrowIfFailed(
                 D3D12SerializeVersionedRootSignature(
                  &desc,
                  signature.ReleaseAndGetAddressOf(),
                  error.ReleaseAndGetAddressOf() ) );

        if (error) { OutputDebugStringA( static_cast<const char*>( error->GetBufferPointer() ) ); }

        DX::ThrowIfFailed(
                 dev->CreateRootSignature(
                  0,
                  signature->GetBufferPointer(),
                  signature->GetBufferSize(),
                  IID_PPV_ARGS(m_local_root_signature_.ReleaseAndGetAddressOf() ) ) );
    }

    void D3D12RaytracingShader::InitializeShaderTable(const Resources::RaytracingShader* shader, ID3D12Device5* dev)
    {
        const auto& export_targets = shader->GetHasExport();
        const bool  hit_merged[RAY_SHADER_REC_MAX]
        {
            export_targets[RAY_SHADER_GEN],
            export_targets[RAY_SHADER_ANY_HIT] || export_targets[RAY_SHADER_CLOSEST_HIT],
            export_targets[RAY_SHADER_MISS]
        };
        
        SetShaderRecordSizes(shader->GetShaderRecordSizes());

        const auto& shader_record_sizes = shader->GetShaderRecordSizes();
        const auto& upload_heap         = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD );
        
        for (size_t i = 0; i < std::size(hit_merged); ++i)
        {
            if (hit_merged)
            {
                const auto& buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(
                Align(shader_record_sizes[i],
                    D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) );
            
                DX::ThrowIfFailed(
                    dev->CreateCommittedResource(
                        &upload_heap,
                        D3D12_HEAP_FLAG_NONE,
                        &buffer_desc,
                        D3D12_RESOURCE_STATE_GENERIC_READ,
                        nullptr,
                        IID_PPV_ARGS( m_shader_tables_[i].GetAddressOf() ) ) );
                
                m_allocated_shader_record_size_[i] = GetShaderRecordSizes()[i];

                const wchar_t* key;
                switch (i)
                {
                case RAY_SHADER_REC_GEN:
                    key = g_raytracing_export_names[RAY_SHADER_GEN];
                    break;
                case RAY_SHADER_REC_MISS:
                    key = g_raytracing_export_names[RAY_SHADER_MISS];
                    break;
                case RAY_SHADER_REC_HIT:
                    m_hit_group_name_ = shader->GetHitGroupName();
                    key = m_hit_group_name_.data();
                    break;
                default:
                    throw std::runtime_error("Unknown type iterated");
                }
                
                char* copy_dst = nullptr;
                DX::ThrowIfFailed(m_shader_tables_[i]->Map(0, nullptr, reinterpret_cast<void**>(&copy_dst)));
                std::memcpy(copy_dst, m_raytracing_pso_properties_->GetShaderIdentifier(key), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                m_shader_tables_[i]->Unmap(0, nullptr);
            }
        }
    }
}
#endif