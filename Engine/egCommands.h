#pragma once
#include <wrl/client.h>

namespace Engine
{
  struct CommandPair final
  {
  public:
    explicit CommandPair(
      eCommandTypes                             type, UINT64 ID, UINT64 buffer_idx, const std::wstring& debug_name);

    CommandPair(const CommandPair& other) = delete;
    CommandPair& operator=(const CommandPair& other) = delete;

    void HardReset();
    void SoftReset();

    void               FlagReady();
    [[nodiscard]] bool IsReady();
    [[nodiscard]] bool IsExecuted();

    [[nodiscard]] ID3D12GraphicsCommandList1* GetList() const;
    [[nodiscard]] eCommandTypes               GetType() const;
    [[nodiscard]] UINT64                      GetLatestFenceValue() const;
    [[nodiscard]] UINT64                      GetBufferIndex() const;
    [[nodiscard]] UINT64                      GetID() const;

    void Execute();

  private:
    friend class Engine::Manager::Graphics::D3Device;

    CommandPair() = default;

    [[nodiscard]] UINT64 Signal(const eCommandTypes type) const;

    UINT64                             m_command_id_;
    UINT64                             m_buffer_idx_;
    std::wstring                       m_debug_name_;
    std::atomic<UINT64>                m_latest_fence_value_;

    bool                               m_b_executed_;
    bool                               m_b_ready_;
    std::mutex                         m_ready_mutex_;
    std::mutex                         m_execute_mutex_;

    eCommandTypes                      m_type_;
    ComPtr<ID3D12CommandAllocator>     m_allocator_;
    ComPtr<ID3D12GraphicsCommandList1> m_list_;
  };
}