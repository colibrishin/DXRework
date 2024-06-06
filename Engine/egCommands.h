#pragma once

namespace Engine
{
  struct CommandAwaiter final
  {
    explicit CommandAwaiter(const UINT fence_value);

    void Wait() const;

  private:
    UINT64 m_fence_value_;
  };

  struct CommandPair final
  {
  public:
    explicit CommandPair(const eCommandTypes type, const UINT64 ID, const std::wstring& debug_name = L"");
    CommandPair(const CommandPair& other);

    CommandPair& operator=(const CommandPair& other);

    void WaitAndReset();
    void HardReset() const;
    void SoftReset() const;

    [[nodiscard]] ID3D12GraphicsCommandList1* GetList() const;
    [[nodiscard]] eCommandTypes               GetType() const;
    [[nodiscard]] UINT64                      GetLatestFenceValue() const;

    static CommandAwaiter Execute(CommandPair* pair, const UINT count);

    CommandAwaiter Execute();

  private:
    [[nodiscard]] static UINT64 Signal(const eCommandTypes type);

    UINT64                             m_command_id_;
    std::wstring                       m_debug_name_;
    std::atomic<UINT64>                m_latest_fence_value_;
    eCommandTypes                      m_type_;
    ComPtr<ID3D12CommandAllocator>     m_allocator_;
    ComPtr<ID3D12GraphicsCommandList1> m_list_;
  };
}