#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include "egD3Device.hpp"

namespace Engine::Graphics
{
  template <size_t Alignment, D3D12_HEAP_TYPE HeapProperty, D3D12_RESOURCE_FLAGS ResourceFlags, D3D12_RESOURCE_STATES ResourceState>
  struct GraphicAllocator
  {
  public:
    static void InitializeBuffer(ComPtr<ID3D12Resource>& resource, const size_t type_size, const size_t count)
    {
      const auto& heap_desc   = CD3DX12_HEAP_PROPERTIES(HeapProperty);
      const auto& buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(Align(count * type_size, Alignment), ResourceFlags);

      DX::ThrowIfFailed
        (
         GetD3Device().GetDevice()->CreateCommittedResource
         (
          &heap_desc,
          D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
          &buffer_desc,
          ResourceState,
          nullptr,
          IID_PPV_ARGS(resource.GetAddressOf())
         )
        );
    }

    bool operator()(ComPtr<ID3D12Resource>& resource, const size_t type_size, const size_t count, const size_t allocated_size) const
    {
      if (allocated_size < Align(count * type_size, Alignment))
      {
        InitializeBuffer(resource, type_size, count);

        return true;
      }

      return false;
    }
  };

  template <size_t Alignment>
  struct GraphicUpdateFunction
  {
  public:
    void operator()(const ComPtr<ID3D12Resource>& resource, const void* src_data, const size_t type_size, const size_t count) const
    {
      char*      data          = nullptr;
      const auto src_char_cast = static_cast<const char*>(src_data);

      DX::ThrowIfFailed(resource->Map(0, nullptr, reinterpret_cast<void**>(&data)));
      for (size_t i = 0; i < count; ++i)
      {
        _mm256_memcpy(data + (i * Align(type_size, Alignment)), src_char_cast + (i * type_size), type_size);
      }
      resource->Unmap(0, nullptr);
    }
  };

  template <typename T, typename Allocator, typename UpdateFunc, size_t Alignment>
  class MemoryPool
  {
  public:
    MemoryPool()
      : m_allocated_size_(1),
        m_used_size_(0)
    {
    }

    void Update(const void* src_data, const size_t type_size, size_t count)
    {
      if (count == 0)
      {
        count = 1;
      }

      if (Allocator()(m_resource_, type_size, count, m_allocated_size_))
      {
        m_allocated_size_ = Align(count * type_size, Alignment);
      }

      m_used_size_ = Align(count * type_size, Alignment);
      if (!src_data)
      {
        return;
      }

      UpdateFunc()(m_resource_, src_data, type_size, count);
    }

    [[nodiscard]] T& GetBaseResource()
    {
      return m_resource_;
    }

    [[nodiscard]] const T& GetBaseResource() const
    {
      return m_resource_;
    }

  protected:
    T m_resource_;
    size_t m_allocated_size_;
	  size_t m_used_size_;
  };

  template <size_t Alignment, D3D12_HEAP_TYPE HeapProperty, D3D12_RESOURCE_FLAGS ResourceFlags, D3D12_RESOURCE_STATES ResourceState>
  class GraphicMemoryPool :
    public MemoryPool<ComPtr<ID3D12Resource>, GraphicAllocator<Alignment, HeapProperty, ResourceFlags, ResourceState>, GraphicUpdateFunction<Alignment>, Alignment>
  {
  public:
    GraphicMemoryPool()
    {
    }

    ~GraphicMemoryPool()
    {
    }

    void Release()
    {
      this->GetBaseResource().Reset();
    }

    [[nodiscard]] ID3D12Resource** GetAddressOf()
    {
      return this->GetBaseResource().GetAddressOf();
    }

    [[nodiscard]] ID3D12Resource* GetResource() const
    {
      return this->GetBaseResource().Get();
    }

    [[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const
    {
      return GetResource()->GetGPUVirtualAddress();
    }
  };
}