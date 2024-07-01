#include "pch.h"
#include "egCommands.h"

namespace Engine
{
  CommandPair::CommandPair(
    const eCommandTypes                 type, const UINT64 ID, const UINT64 buffer_idx, const std::wstring& debug_name)
    : m_command_id_(ID),
      m_buffer_idx_(buffer_idx),
      m_debug_name_(debug_name),
      m_b_executed_(false),
      m_b_ready_(false),
      m_type_(type)
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

    const std::wstring name = debug_name + L" Command Allocator";
    DX::ThrowIfFailed(m_allocator_->SetName(name.c_str()));

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

    DX::ThrowIfFailed(m_list_->SetName(debug_name.c_str()));

    DX::ThrowIfFailed(m_list_->Close());

    if (FAILED(m_list_->QueryInterface(IID_PPV_ARGS(m_list4_.GetAddressOf()))))
    {
      m_list4_ = nullptr;
    }
  }

  void CommandPair::HardReset()
  {
    std::lock_guard<std::mutex> el(m_execute_mutex_);
    std::lock_guard<std::mutex> rl(m_ready_mutex_);

    DX::ThrowIfFailed(m_allocator_->Reset());
    DX::ThrowIfFailed(m_list_->Reset(m_allocator_.Get(), nullptr));
    DX::ThrowIfFailed(m_list_->Close());

    m_b_ready_ = false;
    m_b_executed_ = false;
  }

  void CommandPair::SoftReset()
  {
    std::lock_guard<std::mutex> el(m_execute_mutex_);
    std::lock_guard<std::mutex> rl(m_ready_mutex_);
    DX::ThrowIfFailed(m_list_->Reset(m_allocator_.Get(), nullptr));

    m_b_executed_ = false;
    m_b_ready_ = false;
  }

  void CommandPair::FlagReady(const std::function<void()>& post_execution)
  {
    std::lock_guard<std::mutex> lock(m_ready_mutex_);
    m_b_ready_ = true;

    if (post_execution)
    {
      m_post_execute_function_ = post_execution;
    }

    GetD3Device().m_command_pairs_queued_.store(true);
    GetD3Device().m_command_pairs_queued_.notify_all();
  }

  bool CommandPair::IsReady()
  {
    std::lock_guard<std::mutex> lock(m_ready_mutex_);
    return m_b_ready_;
  }

  bool CommandPair::IsExecuted()
  {
    std::lock_guard<std::mutex> lock(m_execute_mutex_);
    return m_b_executed_;
  }

  ID3D12GraphicsCommandList1* CommandPair::GetList() const
  {
    return m_list_.Get();
  }

  ID3D12GraphicsCommandList4* CommandPair::GetList4() const
  {
    return m_list4_.Get();
  }

  eCommandTypes CommandPair::GetType() const
  {
    return m_type_;
  }

  UINT64 CommandPair::GetLatestFenceValue() const
  {
    return m_latest_fence_value_;
  }

  UINT64 CommandPair::GetBufferIndex() const
  {
    return m_buffer_idx_;
  }

  UINT64 CommandPair::GetID() const
  {
    return m_command_id_;
  }

  void CommandPair::Execute(const bool lock_consuming)
  {
    std::lock_guard<std::mutex> el(m_execute_mutex_);
    std::lock_guard<std::mutex> rl(m_ready_mutex_);

    if (lock_consuming)
    {
      std::lock_guard<std::mutex> ql(GetD3Device().m_command_pairs_mutex_);
      ExecuteImpl();
    }
    else
    {
      ExecuteImpl();
    }
  }

  UINT64 CommandPair::Signal(const eCommandTypes type) const
  {
    if (m_allocator_ == nullptr || m_list_ == nullptr)
    {
      return -1;
    }

    const auto&  fence = GetD3Device().m_fence_;
    auto&        nonce = GetD3Device().m_fence_nonce_[m_buffer_idx_];

    DX::ThrowIfFailed
      (
       GetD3Device().GetCommandQueue(type)->Signal(fence.Get(), ++nonce)
      );

    return nonce;
  }

  void CommandPair::ExecuteImpl()
  {
    if (m_b_executed_)
    {
      return;
    }

    DX::ThrowIfFailed(m_list_->Close());

    const std::vector<ID3D12CommandList*> lists(1, m_list_.Get());

    GetD3Device().GetCommandQueue(m_type_)->ExecuteCommandLists(1, lists.data());

    m_latest_fence_value_ = Signal(m_type_);

    if (GetD3Device().m_fence_->GetCompletedValue() < m_latest_fence_value_)
    {
      const auto& handle = CreateEvent(nullptr, FALSE, FALSE, nullptr);

      DX::ThrowIfFailed
      (
        GetD3Device().m_fence_->SetEventOnCompletion
        (
          m_latest_fence_value_,
          handle
        )
      );

      WaitForSingleObject(handle, INFINITE);
      CloseHandle(handle);
    }

    if (m_post_execute_function_)
    {
      m_post_execute_function_();
    }

    m_b_ready_ = false;
    m_b_executed_ = true;
  }
}
