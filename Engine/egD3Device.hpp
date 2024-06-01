#pragma once
#include <d2d1.h>
#include <d3d12.h>
#include <dxgi1_5.h>
#include <dxgidebug.h>

#include "egGlobal.h"
#include "egManager.hpp"

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

    std::vector<std::pair<D3D12_INPUT_ELEMENT_DESC, std::string>> GenerateInputDescription(
      ID3DBlob* blob
    );
    void CreateSampler(const D3D12_SAMPLER_DESC& description, const D3D12_CPU_DESCRIPTOR_HANDLE& sampler_handle) const;

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

    [[nodiscard]] HANDLE                      GetSwapchainAwaiter() const;
    [[nodiscard]] ID3D12GraphicsCommandList1* GetDirectCommandList(UINT frame_idx = -1) const;
    [[nodiscard]] ID3D12GraphicsCommandList1* GetCopyCommandList(UINT frame_idx = -1) const;
    [[nodiscard]] ID3D12GraphicsCommandList1* GetComputeCommandList(UINT frame_idx = -1)const;
    [[nodiscard]] ID3D12GraphicsCommandList1* GetSubDirectCommandList(UINT frame_idx = -1)const;

    [[nodiscard]] ID3D12CommandQueue* GetDirectCommandQueue() const;
    [[nodiscard]] ID3D12CommandQueue* GetCopyCommandQueue() const;
    [[nodiscard]] ID3D12CommandQueue* GetComputeCommandQueue() const;
    [[nodiscard]] ID3D12CommandQueue* GetSubDirectCommandQueue() const;

    [[nodiscard]] UINT64 GetFrameIndex() const { return m_frame_idx_; }

    void WaitAndReset(const eCommandListIndex type, UINT64 buffer_idx = -1) const;
    void Wait(const eCommandListIndex type, UINT64 buffer_idx = -1);

    void ExecuteCopyCommandList();
    void ExecuteComputeCommandList();
    void ExecuteSubDirectCommandList();

    void CreateTextureFromFile(
        const std::filesystem::path& file_path, 
        ID3D12Resource** res, 
        bool generate_mip) const;
    
  private:
    friend struct SingletonDeleter;
    friend class RenderPipeline;
    friend class ToolkitAPI;

    inline static constexpr eCommandListType s_target_types[] =
    {
      COMMAND_DIRECT,
      COMMAND_COPY,
      COMMAND_COMPUTE,
      COMMAND_SUB_DIRECT
    };
    
    ~D3Device() override = default;

    D3Device() = default;

    void InitializeDevice();
    void InitializeCommandAllocator();
    void InitializeFence();

    void ExecuteDirectCommandList();

    void WaitForEventCompletion(UINT64 buffer_idx) const;
    void WaitForBackBuffer() const;
    
    void CleanupCommandList();
    void WaitNextFrame();

  private:
    [[nodiscard]] ID3D12CommandAllocator* GetDirectCommandAllocator(UINT frame_idx = -1) const;
    [[nodiscard]] ID3D12CommandAllocator* GetCopyCommandAllocator(UINT frame_idx = -1) const;
    [[nodiscard]] ID3D12CommandAllocator* GetComputeCommandAllocator(UINT frame_idx = -1) const;
    [[nodiscard]] ID3D12CommandAllocator* GetSubDirectCommandAllocator(UINT frame_idx = -1) const;

    void Reset(const eCommandListIndex type, const UINT64 buffer_idx = -1) const;
    void Signal(const eCommandListIndex type, UINT64 buffer_idx = -1);
    UINT64 GetFenceValue(UINT64 buffer_idx = -1) const;

    HWND m_hwnd_ = nullptr;

    ComPtr<ID3D12Device2> m_device_ = nullptr;

    UINT s_video_card_memory_        = 0;
    UINT s_refresh_rate_numerator_   = 0;
    UINT s_refresh_rate_denominator_ = 0;

    DXGI_ADAPTER_DESC s_video_card_desc_ = {};

    ComPtr<IDXGISwapChain4> m_swap_chain_ = nullptr;

    ComPtr<ID3D12Fence>              m_fence_       = nullptr;
    HANDLE                           m_fence_event_ = nullptr;
    std::atomic<UINT64>*             m_fence_nonce_;

    UINT64 m_frame_idx_ = 0;

    std::array<ComPtr<ID3D12CommandQueue>, COMMAND_IDX_COUNT>                           m_command_queues_ = {};
    std::map<UINT64, std::array<ComPtr<ID3D12CommandAllocator>, COMMAND_IDX_COUNT>>     m_command_allocators_;
    std::map<UINT64, std::array<ComPtr<ID3D12GraphicsCommandList1>, COMMAND_IDX_COUNT>> m_command_lists_;

    XMMATRIX s_world_matrix_      = {};
    Matrix   m_projection_matrix_ = {};
    Matrix   m_ortho_matrix_      = {};
  };
} // namespace Engine::Manager::Graphics
