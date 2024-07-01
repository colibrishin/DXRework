#pragma once

namespace Engine::Resources
{
  template <typename T>
  void CreateShaderTable(
      ID3D12StateObjectProperties* properties,
      const wchar_t* entrypoint_name,
      ID3D12Resource** shader_table_resource)
  {
    const auto& upload_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    const auto& desc = CD3DX12_RESOURCE_DESC::Buffer
    (
        Align(sizeof(T), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES)
    );

    T empty{};

    _mm256_memcpy
      (
          empty.shaderId, 
          properties->GetShaderIdentifier(entrypoint_name), 
          D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
      );

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateCommittedResource
       (
        &upload_heap,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(shader_table_resource)
       )
      );

    char* data = nullptr;
    DX::ThrowIfFailed((*shader_table_resource)->Map(0, nullptr, reinterpret_cast<void**>(&data)));
    _mm256_memcpy(data, &empty, sizeof(T));
    (*shader_table_resource)->Unmap(0, nullptr);
  }

  template <typename T>
  void CreateShaderTables(
      ID3D12StateObjectProperties* properties,
      std::vector<const wchar_t*> entrypoints,
      ID3D12Resource** shader_table_resource)
  {
    const auto& upload_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    const auto& desc = CD3DX12_RESOURCE_DESC::Buffer
    (
        Align(sizeof(T) * entrypoints.size(), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES)
    );

    std::vector<T> empty{};
    empty.resize(entrypoints.size());

    for (size_t i = 0; i < entrypoints.size(); ++i)
    {
      _mm256_memcpy
      (
          empty[i].shaderId, 
          properties->GetShaderIdentifier(entrypoints[i]), 
          D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
      );
    }

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateCommittedResource
       (
        &upload_heap,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(shader_table_resource)
       )
      );

    char* data = nullptr;
    DX::ThrowIfFailed((*shader_table_resource)->Map(0, nullptr, reinterpret_cast<void**>(&data)));
    _mm256_memcpy(data, empty.data(), sizeof(T) * entrypoints.size());
    (*shader_table_resource)->Unmap(0, nullptr);
  }
  
}