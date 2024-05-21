#pragma once
#include <d2d1.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>
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
      pDXGIDebug->ReportLiveObjects(DXGI_DEBUG_D3D11, DXGI_DEBUG_RLO_DETAIL);
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
    
  private:
    friend struct SingletonDeleter;
    friend class RenderPipeline;
    friend class ToolkitAPI;
    ~D3Device() override = default;

      DX::ThrowIfFailed
        (
         m_device_->CreateCommittedResource
         (
          &default_prop,
          D3D12_HEAP_FLAG_NONE,
          &resource_desc,
          D3D12_RESOURCE_STATE_COPY_DEST,
          nullptr,
          IID_PPV_ARGS(buffer)
         )
        );

    static void UpdateBuffer(UINT64 size, const void* src, ID3D12Resource* dst);

    void InitializeDevice();
    void InitializeRenderTargetView();
    void InitializeCommandAllocator();
    void InitializeFence();
    void InitializeD2D();
    void InitializeDepthStencil();

    void WaitForSingleCompletion();
    
    void WaitForPreviousFrame();
    void FrameBegin();
    void Present() const;

  private:
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

    XMMATRIX s_world_matrix_      = {};
    Matrix   m_projection_matrix_ = {};
    Matrix   m_ortho_matrix_      = {};
  };
} // namespace Engine::Manager::Graphics
