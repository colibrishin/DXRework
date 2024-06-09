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
    [[nodiscard]] ID3D12GraphicsCommandList1* GetCommandList(const eCommandList list_enum, UINT frame_idx = -1);

    [[nodiscard]] ID3D12CommandQueue* GetCommandQueue(const eCommandList list) const;
    ID3D12CommandQueue*               GetCommandQueue(eCommandTypes type) const;

    [[nodiscard]] UINT64 GetFrameIndex() const { return m_frame_idx_; }

    [[nodiscard]] CommandPair& AcquireCommandPair(const std::wstring& debug_name, UINT64 buffer_idx = -1);
    bool                       IsCommandPairAvailable(UINT64 buffer_idx = -1) const;
    void                       Flush();

    void WaitAndReset(const eCommandList list, UINT64 buffer_idx = -1);
    void Wait(UINT64 buffer_idx = -1) const;
    void Signal(const eCommandTypes type, UINT64 buffer_idx = -1) const;

    void CreateTextureFromFile(
        const std::filesystem::path& file_path, 
        ID3D12Resource** res, 
        bool generate_mip) const;

    void ExecuteCommandList(const eCommandList list) const;
    
  private:
    friend struct SingletonDeleter;
    friend struct Engine::CommandPair;
    friend struct Engine::CommandAwaiter;
    friend class RenderPipeline;
    friend class ToolkitAPI;

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
    
    ~D3Device() override = default;

    D3Device() = default;

    void InitializeDevice();
    void InitializeCommandAllocator();
    void InitializeFence();

    void WaitForEventCompletion(UINT64 buffer_idx) const;
    
    void WaitNextFrame();

  private:
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

    std::atomic<UINT64>              m_command_ids_ = 0;
    UINT64 m_frame_idx_ = 0;

    std::map<FrameIndex, std::vector<CommandPair>>             m_command_pairs_           = {};
    std::vector<CommandPair>                                   m_command_pairs_generated_ = {};
    std::array<ComPtr<ID3D12CommandQueue>, COMMAND_TYPE_COUNT> m_command_queues_          = {};

    XMMATRIX s_world_matrix_      = {};
    Matrix   m_projection_matrix_ = {};
    Matrix   m_ortho_matrix_      = {};
  };
} // namespace Engine::Manager::Graphics
