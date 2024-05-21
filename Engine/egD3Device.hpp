#pragma once
#include <d2d1.h>
#include <d3d12.h>
#include <dxgi1_5.h>
#include <dxgidebug.h>
#include "egManager.hpp"

#ifdef _DEBUG
#include "dxgidebug.h"
#endif

namespace Engine::Manager::Graphics
{
  using namespace DirectX;
  using Microsoft::WRL::ComPtr;
  using namespace Engine::Graphics;

  const std::map<GUID, eShaderType, GUIDComparer> g_shader_enum_type_map =
    {
    {__uuidof(ID3D11VertexShader), SHADER_VERTEX},
    {__uuidof(ID3D11PixelShader), SHADER_PIXEL},
    {__uuidof(ID3D11GeometryShader), SHADER_GEOMETRY},
    {__uuidof(ID3D11ComputeShader), SHADER_COMPUTE},
    {__uuidof(ID3D11HullShader), SHADER_HULL},
    {__uuidof(ID3D11DomainShader), SHADER_DOMAIN}
  };

  const std::unordered_map<std::wstring, eShaderType> g_shader_type_map = {
    {L"vs", SHADER_VERTEX}, {L"ps", SHADER_PIXEL}, {L"gs", SHADER_GEOMETRY},
    {L"cs", SHADER_COMPUTE}, {L"hs", SHADER_HULL}, {L"ds", SHADER_DOMAIN}
  };

  const std::unordered_map<eShaderType, std::string> g_shader_target_map = {
    {SHADER_VERTEX, "vs_5_0"}, {SHADER_PIXEL, "ps_5_0"},
    {SHADER_GEOMETRY, "gs_5_0"}, {SHADER_COMPUTE, "cs_5_0"},
    {SHADER_HULL, "hs_5_0"}, {SHADER_DOMAIN, "ds_5_0"}
  };

  class D3Device final : public Abstract::Singleton<D3Device, HWND>
  {
  public:
    D3Device(SINGLETON_LOCK_TOKEN)
      : Singleton() {}

    void Initialize(HWND hWnd) override;

    static void DEBUG_MEMORY()
    {
#ifdef _DEBUG
      HMODULE hModule                   = GetModuleHandleW(L"dxgidebug.dll");
      auto    DXGIGetDebugInterfaceFunc =
        reinterpret_cast<decltype(DXGIGetDebugInterface)*>(
          GetProcAddress(hModule, "DXGIGetDebugInterface"));

      IDXGIDebug* pDXGIDebug;
      DXGIGetDebugInterfaceFunc(IID_PPV_ARGS(&pDXGIDebug));
      pDXGIDebug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_DETAIL);
#endif
    }

    static float GetAspectRatio();
    void         UpdateRenderTarget() const;

    void CreateShaderResourceView(
      ID3D12Resource * resource, const D3D12_SHADER_RESOURCE_VIEW_DESC & srv, ID3D11ShaderResourceView ** view
    ) const;

    std::vector<std::pair<D3D12_INPUT_ELEMENT_DESC, std::string>> GenerateInputDescription(
      ID3DBlob* blob
    );
    void CreateSampler(const D3D12_SAMPLER_DESC& description, const D3D12_CPU_DESCRIPTOR_HANDLE& handle) const;

    void CreateConstantBufferView(
      const UINT slot, const D3D12_CONSTANT_BUFFER_VIEW_DESC & description
    ) const;

    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PostRender(const float& dt) override;
    void PostUpdate(const float& dt) override;

    const Matrix& GetProjectionMatrix() const { return m_projection_matrix_; }
    const Matrix& GetOrthogonalMatrix() const { return m_ortho_matrix_; }

    ID3D12Device* GetDevice() const { return m_device_.Get(); }
    
    void __fastcall CopySwapchain(ID3D12Resource* buffer, ID3D12GraphicsCommandList* command_list) const;

    [[nodiscard]] HANDLE                            GetSwapchainAwaiter() const;
    [[nodiscard]] CD3DX12_CPU_DESCRIPTOR_HANDLE     GetRTVHandle() const;
    [[nodiscard]] CD3DX12_CPU_DESCRIPTOR_HANDLE     GetDSVHandle() const;
    [[nodiscard]] D3D12_FEATURE_DATA_ROOT_SIGNATURE GetRootSignatureFeature() const;

    [[nodiscard]] ID3D12GraphicsCommandList* GetCommandList() const;

    template <typename T>
    void CreateBuffer1D
    (
      ID3D12Resource** buffer,
      const void* data,
      const UINT64 size,
      const D3D12_RESOURCE_BARRIER& barrier,
      const std::wstring& name = L""
      ) const
    {
      const auto& default_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
      const auto& resource_desc = CD3DX12_RESOURCE_DESC::Buffer(size);

      DX::ThrowIfFailed
        (
         GetD3Device().GetDevice()->CreateCommittedResource
         (
          &default_prop,
          D3D12_HEAP_FLAG_NONE,
          &resource_desc,
          D3D12_RESOURCE_STATE_COPY_DEST,
          nullptr,
          IID_PPV_ARGS(buffer)
         )
        );

      DX::ThrowIfFailed((*buffer)->SetName(name.c_str()));

      ComPtr<ID3D12Resource> upload_buffer;
      const auto& upload_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

      DX::ThrowIfFailed
        (
         m_device_->CreateCommittedResource
         (
          &upload_prop,
          D3D12_HEAP_FLAG_NONE,
          &resource_desc,
          D3D12_RESOURCE_STATE_GENERIC_READ,
          nullptr,
          IID_PPV_ARGS(upload_buffer.GetAddressOf())
         )
        );

      const D3D12_SUBRESOURCE_DATA data_desc
      {
        .pData = data,
        .RowPitch = sizeof(T) * size,
        .SlicePitch = sizeof(T) * size
      };

      UpdateSubresources
      (
        m_command_list_.Get(),
        *buffer,
        upload_buffer.Get(),
        0,
        0,
        1,
        &data_desc
      );

      GetD3Device().GetCommandList()->ResourceBarrier(1, &barrier);
      DX::ThrowIfFailed(m_command_list_->Close());
      GetD3Device().ForceExecuteCommandList();
      GetD3Device().WaitForUploadCompletion();
    }
    
  private:
    friend struct SingletonDeleter;
    friend class RenderPipeline;
    friend class ToolkitAPI;

    inline static constexpr eCommandListType s_target_types[] =
    {
      COMMAND_DIRECT,
      COMMAND_COPY,
      COMMAND_COMPUTE,
      COMMAND_TOOLKIT
    };
    
    ~D3Device() override = default;

    D3Device() = default;
    
    static void UpdateBuffer(UINT64 size, const void* src, ID3D12Resource* dst);

    void InitializeDevice();
    void InitializeRenderTargetView();
    void InitializeCommandAllocator();
    void InitializeFence();
    void InitializeD2D();
    void InitializeDepthStencil();
    
    void WaitForPreviousFrame();
    void WaitForUploadCompletion();
    void WaitForSingleCompletion();
    void ForceExecuteCommandList() const;
    
    void FrameBegin();
    void Present() const;

    HWND m_hwnd_ = nullptr;

    ComPtr<ID3D12Device>        m_device_  = nullptr;
    ComPtr<IDXGISurface1>     m_surface_                = nullptr;
    ComPtr<ID2D1RenderTarget> m_d2d_render_target_view_ = nullptr;

    UINT s_video_card_memory_        = 0;
    UINT s_refresh_rate_numerator_   = 0;
    UINT s_refresh_rate_denominator_ = 0;

    DXGI_ADAPTER_DESC s_video_card_desc_ = {};

    ComPtr<IDXGISwapChain4>      m_swap_chain_      = nullptr;
    ComPtr<ID3D12CommandQueue>   m_command_queue_   = nullptr;
    
    ComPtr<ID3D12DescriptorHeap> m_rtv_descriptor_heap_ = nullptr;
    ComPtr<ID3D12DescriptorHeap> m_dsv_descriptor_heap_ = nullptr;
    ComPtr<ID3D12DescriptorHeap> m_buffer_descriptor_heap_ = nullptr;

    std::vector<ComPtr<ID3D12Fence>> m_fences_       = {nullptr,};
    HANDLE                           m_fence_event_  = nullptr;
    std::vector<UINT64>              m_fences_nonce_ = {0,};

    UINT64                                      m_frame_idx_         = 0;
    UINT64                                      m_rtv_desc_size_     = 0;
    std::vector<ComPtr<ID3D12Resource>>         m_render_targets_    = {nullptr,};
    std::vector<ComPtr<ID3D12CommandAllocator>> m_command_allocator_ = {nullptr,};

    // todo: multi pipeline?
    ComPtr<ID3D12GraphicsCommandList> m_command_list_ = {nullptr, };
    
    D3D11_VIEWPORT s_viewport_{};

    UINT64 m_frame_idx_ = 0;

    std::array<ComPtr<ID3D12CommandQueue>, COMMAND_IDX_COUNT>                           m_command_queues_ = {};
    std::map<UINT64, std::array<ComPtr<ID3D12CommandAllocator>, COMMAND_IDX_COUNT>>     m_command_allocators_;
    std::map<UINT64, std::array<ComPtr<ID3D12GraphicsCommandList1>, COMMAND_IDX_COUNT>> m_command_lists_;

    XMMATRIX s_world_matrix_      = {};
    Matrix   m_projection_matrix_ = {};
    Matrix   m_ortho_matrix_      = {};
  };
} // namespace Engine::Manager::Graphics
