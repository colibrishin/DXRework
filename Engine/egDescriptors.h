#pragma once

namespace Engine
{
  struct DescriptorHandler;

  struct DescriptorPtrImpl final
  {
  public:
    DescriptorPtrImpl();

    DescriptorPtrImpl(DescriptorPtrImpl&& other) noexcept;
    DescriptorPtrImpl& operator=(DescriptorPtrImpl&& other) noexcept;

    DescriptorPtrImpl(const DescriptorPtrImpl& other) = delete;
    DescriptorPtrImpl& operator=(const DescriptorPtrImpl& other) = delete;

    ~DescriptorPtrImpl();

    [[nodiscard]] bool IsValid() const { return m_offset_ != -1; }
    void Release() const;

    [[nodiscard]] ID3D12DescriptorHeap* GetMainDescriptorHeap() const;

    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const { return m_cpu_handle_; }
    [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const { return m_gpu_handle_; }

    void SetSampler(const D3D12_CPU_DESCRIPTOR_HANDLE& sampler, const UINT slot) const;
    void SetConstantBuffer(const D3D12_CPU_DESCRIPTOR_HANDLE& cbv, const UINT slot) const;
    void SetShaderResource(const D3D12_CPU_DESCRIPTOR_HANDLE& srv_handle, const UINT slot) const;
    void SetShaderResources(UINT slot, UINT count, const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& data) const;
    void SetUnorderedAccess(const D3D12_CPU_DESCRIPTOR_HANDLE& uav, const UINT slot) const;
    void BindGraphic(const Weak<CommandPair> & w_cmd) const;

  private:
    friend struct DescriptorHandler;

    explicit DescriptorPtrImpl(
      DescriptorHandler*                 handler, const INT64& offset, const D3D12_CPU_DESCRIPTOR_HANDLE& cpu_handle,
      const D3D12_GPU_DESCRIPTOR_HANDLE& gpu_handle,
      const D3D12_CPU_DESCRIPTOR_HANDLE& cpu_sampler_handle, const D3D12_GPU_DESCRIPTOR_HANDLE& gpu_sampler_handle,
      const UINT                         buffer_descriptor_size, const UINT                     sampler_descriptor_size
    )
      : m_handler_(handler),
        m_offset_(offset),
        m_cpu_handle_(cpu_handle),
        m_gpu_handle_(gpu_handle),
        m_cpu_sampler_handle_(cpu_sampler_handle),
        m_gpu_sampler_handle_(gpu_sampler_handle),
        m_buffer_descriptor_size_(buffer_descriptor_size),
        m_sampler_descriptor_size_(sampler_descriptor_size) {}

    DescriptorHandler* m_handler_;
    INT64 m_offset_;

    D3D12_CPU_DESCRIPTOR_HANDLE m_cpu_handle_;
    D3D12_GPU_DESCRIPTOR_HANDLE m_gpu_handle_;

    D3D12_CPU_DESCRIPTOR_HANDLE m_cpu_sampler_handle_;
    D3D12_GPU_DESCRIPTOR_HANDLE m_gpu_sampler_handle_;

    UINT m_buffer_descriptor_size_;
    UINT m_sampler_descriptor_size_;
  };

  struct DescriptorHandler final
  {
  public:
    void UpdateHeaps(UINT size);
    DescriptorHandler(const UINT size);

    DescriptorPtr Acquire();
    void          Release(const DescriptorPtrImpl& handles);
    bool          IsAvailable() const;

    [[nodiscard]] ID3D12DescriptorHeap* GetMainDescriptorHeap() const { return m_main_descriptor_heap_.Get(); }
    [[nodiscard]] ID3D12DescriptorHeap* GetMainSamplerDescriptorHeap() const { return m_main_sampler_descriptor_heap_.Get(); }

  private:
    UINT m_size_;
    std::vector<bool> m_used_slots_;
    std::vector<Strong<DescriptorPtrImpl>> m_descriptors_;

    ComPtr<ID3D12DescriptorHeap> m_main_descriptor_heap_;
    ComPtr<ID3D12DescriptorHeap> m_main_sampler_descriptor_heap_;

    UINT m_buffer_size_;
    UINT m_sampler_size_;
  };
}