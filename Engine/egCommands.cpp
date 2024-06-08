#include "pch.h"
#include "egCommands.h"

namespace Engine
{
  CommandAwaiter::CommandAwaiter(const UINT fence_value)
    : m_fence_value_(fence_value) {}

  void CommandAwaiter::Wait() const
  {
    const auto& fence       = GetD3Device().m_fence_;
    const auto& idx         = GetD3Device().m_frame_idx_;

    if (fence->GetCompletedValue() < m_fence_value_)
    {
      const auto& handle = CreateEvent(nullptr, FALSE, FALSE, nullptr);

      DX::ThrowIfFailed
        (
         fence->SetEventOnCompletion
         (
          m_fence_value_,
          handle
         )
        );

      WaitForSingleObject(handle, INFINITE);
      CloseHandle(handle);
    }
  }

  CommandPair::CommandPair(const eCommandTypes type, const UINT64 ID, const std::wstring& debug_name) :
    m_debug_name_(debug_name), m_type_(type), m_command_id_(ID)
  {
    constexpr eCommandNativeType conversion[]
    {
      COMMAND_NATIVE_DIRECT,
      COMMAND_NATIVE_COPY,
      COMMAND_NATIVE_COMPUTE
    };

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateCommandAllocator
       (
        static_cast<D3D12_COMMAND_LIST_TYPE>(conversion[type]),
        IID_PPV_ARGS(m_allocator_.GetAddressOf())
       )
      );

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateCommandList
       (
        0,
        static_cast<D3D12_COMMAND_LIST_TYPE>(conversion[type]),
        m_allocator_.Get(),
        nullptr,
        IID_PPV_ARGS(m_list_.GetAddressOf())
       )
      );

    DX::ThrowIfFailed(m_list_->Close());
  }

  CommandPair::CommandPair(const CommandPair& other)
  {
    m_type_   = other.m_type_;
    m_debug_name_ = other.m_debug_name_;
    m_command_id_ = other.m_command_id_;
    m_latest_fence_value_.store(other.m_latest_fence_value_.load());
    m_allocator_ = other.m_allocator_;
    m_list_      = other.m_list_;
  }

  CommandPair& CommandPair::operator=(const CommandPair& other)
  {
    if (this != &other)
    {
      m_type_   = other.m_type_;
      m_debug_name_ = other.m_debug_name_;
      m_command_id_ = other.m_command_id_;
      m_latest_fence_value_.store(other.m_latest_fence_value_.load());
      m_allocator_ = other.m_allocator_;
      m_list_      = other.m_list_;
    }

    return *this;
  }

  void CommandPair::WaitAndReset()
  {
    const auto& fence       = GetD3Device().m_fence_;
    const auto& idx         = GetD3Device().m_frame_idx_;

    if (fence->GetCompletedValue() < m_latest_fence_value_)
    {
      const auto& handle = CreateEvent(nullptr, FALSE, FALSE, nullptr);

      DX::ThrowIfFailed
        (
         fence->SetEventOnCompletion
         (
          m_latest_fence_value_,
          handle
         )
        );

      WaitForSingleObject(handle, INFINITE);
      CloseHandle(handle);
    }

    DX::ThrowIfFailed(m_list_->Reset(m_allocator_.Get(), nullptr));
  }

  void CommandPair::HardReset() const
  {
    DX::ThrowIfFailed(m_allocator_->Reset());
    DX::ThrowIfFailed(m_list_->Reset(m_allocator_.Get(), nullptr));
    DX::ThrowIfFailed(m_list_->Close());
  }

  void CommandPair::SoftReset() const
  {
    DX::ThrowIfFailed(m_list_->Reset(m_allocator_.Get(), nullptr));
  }

  ID3D12GraphicsCommandList1* CommandPair::GetList() const
  {
    return m_list_.Get();
  }

  eCommandTypes CommandPair::GetType() const
  {
    return m_type_;
  }

  UINT64 CommandPair::GetLatestFenceValue() const
  {
    return m_latest_fence_value_;
  }

  CommandAwaiter CommandPair::Execute(CommandPair* pair, const UINT count)
  {
    const auto                      base_type = pair[0].m_type_;
    std::vector<ID3D12CommandList*> lists(count);

    for (UINT i = 0; i < count; ++i)
    {
      if (pair[i].m_type_ != base_type)
      {
        throw std::runtime_error("Command list types do not match.");
        break;
      }

      lists[i] = pair[i].GetList();

      DX::ThrowIfFailed(pair[i].GetList()->Close());
    }

    GetD3Device().GetCommandQueue(base_type)->ExecuteCommandLists(count, lists.data());

    UINT last_signal = 0;

    for (UINT i = 0; i < count; ++i)
    {
      pair[i].m_latest_fence_value_ = pair[i].Signal(base_type);
      last_signal = pair[i].m_latest_fence_value_;
    }

    return CommandAwaiter(last_signal);
  }

  CommandAwaiter CommandPair::Execute()
  {
    DX::ThrowIfFailed(m_list_->Close());

    const std::vector<ID3D12CommandList*> lists(1, m_list_.Get());

    GetD3Device().GetCommandQueue(m_type_)->ExecuteCommandLists(1, lists.data());

    m_latest_fence_value_ = Signal(m_type_);

    return CommandAwaiter(m_latest_fence_value_);
  }

  UINT64 CommandPair::Signal(const eCommandTypes type)
  {
    const auto& fence = GetD3Device().m_fence_;
    const auto& idx   = GetD3Device().m_frame_idx_;
    auto&       nonce = GetD3Device().m_fence_nonce_[idx];

    DX::ThrowIfFailed
      (
       GetD3Device().GetCommandQueue(type)->Signal(fence.Get(), ++nonce)
      );

    return nonce;
  }
}
