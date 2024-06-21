#pragma once
#include <d2d1.h>
#include <d3d12.h>
#include <dxgi1_5.h>
#include <dxgidebug.h>

#include "egGlobal.h"
#include "egManager.hpp"
#include "egCommands.h"

#ifdef _DEBUG
#include "dxgidebug.h"
#endif

namespace Engine::Manager::Graphics
{
  using namespace DirectX;
  using Microsoft::WRL::ComPtr;
  using namespace Engine::Graphics;

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
    ID3D12Resource*  GetRenderTarget(UINT64 frame_idx);

    static void DEBUG_DEVICE_REMOVED(ID3D12Device* device)
    {
      if constexpr (g_debug_device_removal)
      {
        ComPtr<ID3D12DeviceRemovedExtendedData> dred;
        DX::ThrowIfFailed(device->QueryInterface(IID_PPV_ARGS(&dred)));

        D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT dred_breadcrumbs;
        D3D12_DRED_PAGE_FAULT_OUTPUT       dred_page_fault;
        DX::ThrowIfFailed(dred->GetAutoBreadcrumbsOutput(&dred_breadcrumbs));
        DX::ThrowIfFailed(dred->GetPageFaultAllocationOutput(&dred_page_fault));
        __debugbreak();
      }
    }

    static void DEBUG_MEMORY()
    {
      if constexpr (g_debug)
      {
        HMODULE hModule                   = GetModuleHandleW(L"dxgidebug.dll");
        auto    DXGIGetDebugInterfaceFunc =
          reinterpret_cast<decltype(DXGIGetDebugInterface)*>(
            GetProcAddress(hModule, "DXGIGetDebugInterface"));

        IDXGIDebug* pDXGIDebug;
        DXGIGetDebugInterfaceFunc(IID_PPV_ARGS(&pDXGIDebug));
        pDXGIDebug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_DETAIL);
      }
    }

    static float GetAspectRatio();

    std::vector<std::pair<D3D12_INPUT_ELEMENT_DESC, std::string>> GenerateInputDescription(
      ID3DBlob* blob
    );

    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PostRender(const float& dt) override;
    void PostUpdate(const float& dt) override;

    const Matrix& GetProjectionMatrix() const { return m_projection_matrix_; }
    const Matrix& GetOrthogonalMatrix() const { return m_ortho_matrix_; }

    [[nodiscard]] ID3D12Device* GetDevice() const { return m_device_.Get(); }

    [[nodiscard]] HANDLE                      GetSwapchainAwaiter() const;
    [[nodiscard]] ID3D12GraphicsCommandList1* GetCommandList(const eCommandList list_enum, UINT frame_idx = -1) const;

    [[nodiscard]] ID3D12CommandQueue* GetCommandQueue(const eCommandList list) const;
    [[nodiscard]] ID3D12CommandQueue* GetCommandQueue(eCommandTypes type) const;

    [[nodiscard]] UINT64 GetFrameIndex() const { return m_frame_idx_; }

    [[nodiscard]] Weak<CommandPair> AcquireCommandPair(
      const std::wstring& debug_name, UINT64 buffer_idx = -1
    );
    bool                      IsCommandPairAvailable(UINT64 buffer_idx = -1) const;

    void WaitAndReset(const eCommandList list, UINT64 buffer_idx = -1) const;
    void Wait(UINT64 buffer_idx = -1) const;
    void Signal(const eCommandTypes type, UINT64 buffer_idx = -1) const;

    void CreateTextureFromFile(
        const std::filesystem::path& file_path, 
        ID3D12Resource** res, 
        bool generate_mip) const;

    void DefaultRenderTarget(const Weak<CommandPair>& w_cmd) const;
    void CopyBackBuffer(const Weak<CommandPair>& w_cmd, ID3D12Resource* resource) const;
    void ClearRenderTarget(bool barrier = true);

    [[nodiscard]] ID3D12DescriptorHeap* GetRTVHeap() const;
    [[nodiscard]] ID3D12DescriptorHeap* GetDSVHeap() const;
    [[nodiscard]] UINT                  GetRTVHeapSize() const;

  private:
    friend struct SingletonDeleter;
    friend struct Engine::CommandPair;
    friend struct Engine::DescriptorHandler;
    friend class RenderPipeline;
    friend class ToolkitAPI;
    friend class GarbageCollector;

    inline static constexpr eCommandNativeType s_native_target_types[] =
    {
      COMMAND_NATIVE_DIRECT,
      COMMAND_NATIVE_DIRECT,
      COMMAND_NATIVE_DIRECT,
      COMMAND_NATIVE_DIRECT,
      COMMAND_NATIVE_COPY,
      COMMAND_NATIVE_COMPUTE
    };

    inline static constexpr eCommandTypes s_target_types[] =
    {
      COMMAND_TYPE_DIRECT,
      COMMAND_TYPE_DIRECT,
      COMMAND_TYPE_DIRECT,
      COMMAND_TYPE_DIRECT,
      COMMAND_TYPE_COPY,
      COMMAND_TYPE_COMPUTE
    };
    
    ~D3Device() override;

    D3Device() = default;

    void InitializeDevice();
    void InitializeCommandAllocator();
    void InitializeFence();
    void InitializeConsumer();

    void WaitForEventCompletion(UINT64 buffer_idx) const;
    void WaitForCommandsCompletion();
    void WaitNextFrame();
    void ConsumeCommands();

    UINT64 GetFenceValue(UINT64 buffer_idx = -1) const;

    HWND m_hwnd_ = nullptr;

    ComPtr<ID3D12Device2> m_device_ = nullptr;

    UINT s_video_card_memory_        = 0;
    UINT s_refresh_rate_numerator_   = 0;
    UINT s_refresh_rate_denominator_ = 0;

    DXGI_ADAPTER_DESC s_video_card_desc_ = {};

    ComPtr<IDXGISwapChain4> m_swap_chain_ = nullptr;

    std::vector<ComPtr<ID3D12Resource>> m_render_targets_;
    ComPtr<ID3D12DescriptorHeap>        m_rtv_heap_;
    UINT                                m_rtv_heap_size_;

    ComPtr<ID3D12Resource>       m_depth_stencil_;
    ComPtr<ID3D12DescriptorHeap> m_dsv_heap_;
    

    ComPtr<ID3D12Fence>              m_fence_       = nullptr;
    HANDLE                           m_fence_event_ = nullptr;
    std::atomic<UINT64>*             m_fence_nonce_;

    std::atomic<UINT64>              m_command_ids_ = 0;
    UINT64 m_frame_idx_ = 0;

    std::map<FrameIndex, std::vector<Strong<CommandPair>>>     m_command_pairs_;
    std::array<ComPtr<ID3D12CommandQueue>, COMMAND_TYPE_COUNT> m_command_queues_;

    std::mutex                            m_command_pairs_mutex_;
    std::map<UINT64, Strong<CommandPair>> m_command_pairs_generated_;
    std::atomic<UINT64>                   m_command_pairs_count_ = 0;

    std::thread                  m_command_consumer_;
    std::atomic<bool>            m_command_consumer_running_ = false;

    std::mutex                   m_command_producer_mutex_;
    std::condition_variable      m_command_producer_cv_;

    XMMATRIX s_world_matrix_      = {};
    Matrix   m_projection_matrix_ = {};
    Matrix   m_ortho_matrix_      = {};
  };
} // namespace Engine::Manager::Graphics
