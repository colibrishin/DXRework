#include "../Public/CommandPair.h"

#pragma comment(lib, "d3d12.lib")

#include "ThrowIfFailed.h"

namespace Engine
{
	CommandPair::CommandPair
	(
		CommandPairPool*    pool,
		const D3D12_COMMAND_LIST_TYPE  type,
		const UINT64        ID,
		const UINT64        buffer_idx,
		const std::wstring_view debug_name,
		DescriptorPtr&& heap
	)
		: m_command_id_(ID),
		  m_buffer_idx_(buffer_idx),
		  m_debug_name_(debug_name),
		  m_b_executed_(false),
		  m_b_ready_(false),
		  m_b_disposed_(false),
		  m_type_(type),
		  m_assigned_heap_(std::move(heap))
	{
		if (pool != nullptr)
		{
			m_pool_ = pool;

			DX::ThrowIfFailed
				(
				 pool->m_dev_->CreateCommandAllocator
				 (
				  type,
				  IID_PPV_ARGS(m_allocator_.GetAddressOf())
				 )
				);

			std::wstring name(debug_name);
			name += +L" Command Allocator";
			
			DX::ThrowIfFailed(m_allocator_->SetName(name.data()));

			DX::ThrowIfFailed
					(
					 pool->m_dev_->CreateCommandList
					 (
					  0,
					  type,
					  m_allocator_.Get(),
					  nullptr,
					  IID_PPV_ARGS(m_list_.GetAddressOf())
					 )
					);

			DX::ThrowIfFailed(m_list_->SetName(debug_name.data()));
			DX::ThrowIfFailed(m_list_->Close());

			if (FAILED(m_list_->QueryInterface(IID_PPV_ARGS(m_list4_.GetAddressOf()))))
			{
				m_list4_ = nullptr;
			}
		}
	}

	void CommandPair::SetDisposed()
	{
		std::lock_guard<std::mutex> l(m_critical_mutex_);
		DX::ThrowIfFailed(m_list_->Close());

		m_b_ready_    = false;
		m_b_executed_ = false;
		m_b_disposed_ = true;
	}

	void CommandPair::HardReset()
	{
		std::lock_guard<std::mutex> l(m_critical_mutex_);

		DX::ThrowIfFailed(m_allocator_->Reset());
		DX::ThrowIfFailed(m_list_->Reset(m_allocator_.Get(), nullptr));
		DX::ThrowIfFailed(m_list_->Close());

		m_b_ready_    = false;
		m_b_executed_ = false;
		m_b_disposed_ = false;
	}

	void CommandPair::SoftReset()
	{
		std::lock_guard<std::mutex> l(m_critical_mutex_);

		DX::ThrowIfFailed(m_list_->Reset(m_allocator_.Get(), nullptr));

		m_b_executed_ = false;
		m_b_ready_    = false;
		m_b_disposed_ = false;
	}

	void CommandPair::FlagReady(const std::function<void()>& post_execution)
	{
		std::lock_guard<std::mutex> l(m_critical_mutex_);
		m_b_ready_ = true;

		if (post_execution)
		{
			m_post_execute_function_ = post_execution;
		}
	}

	void CommandPair::Execute() const
	{
		constexpr bool expected = true;
		m_b_executed_.wait(expected);
	}

	bool CommandPair::IsReady()
	{
		std::lock_guard<std::mutex> l(m_critical_mutex_);
		return m_b_ready_;
	}

	bool CommandPair::IsExecuted()
	{
		std::lock_guard<std::mutex> l(m_critical_mutex_);
		return m_b_executed_;
	}

	bool CommandPair::IsDisposed()
	{
		std::lock_guard<std::mutex> l(m_critical_mutex_);
		return m_b_disposed_;
	}

	ID3D12GraphicsCommandList1* CommandPair::GetList() const
	{
		return m_list_.Get();
	}

	ID3D12GraphicsCommandList4* CommandPair::GetList4() const
	{
		return m_list4_.Get();
	}

	ID3D12CommandAllocator* CommandPair::GetAllocator() const
	{
		return m_allocator_.Get();
	}

	D3D12_COMMAND_LIST_TYPE CommandPair::GetType() const
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

	Weak<CommandPair> CommandPairPool::Allocate(
		const D3D12_COMMAND_LIST_TYPE type, const UINT64 id, const UINT64 buffer_idx, const std::wstring_view debug_name, const bool heap_allocation
	)
	{
		std::lock_guard lock(m_mutex_);

		const auto& it = std::ranges::find_if(m_allocation_map_, [](const std::pair<const address_value, bool>& kv) 
		{
			return !kv.second;
		});

		if (it == m_allocation_map_.end())
		{
			return {};
		}

		const address_value available = it->first;

		m_allocation_map_[available] = true;

		if (m_pool_[available]->GetType() == type && m_pool_[available]->GetList())
		{
			m_pool_[available]->m_command_id_ = id;
			m_pool_[available]->m_buffer_idx_ = buffer_idx;
			m_pool_[available]->m_debug_name_ = debug_name;

			if (heap_allocation && m_heap_handler_) 
			{
				m_pool_[available]->m_assigned_heap_ = std::move(m_heap_handler_->Acquire());
			}
			else 
			{
				m_pool_[available]->m_assigned_heap_.reset();
			}

			DX::ThrowIfFailed(m_pool_[available]->m_list_->SetName(debug_name.data()));
		}
		else
		{
			new(m_pool_[available].get()) CommandPair(this, type, id, buffer_idx, debug_name, heap_allocation && m_heap_handler_ ? m_heap_handler_->Acquire() : nullptr);
		}
			
		return m_pool_[available];
	}

	void CommandPairPool::Deallocate(const Weak<CommandPair>& pointer)
	{
		std::lock_guard lock(m_mutex_);

		if (const auto& locked = pointer.lock())
		{
			locked->HardReset();
			locked->m_assigned_heap_.reset();
			m_allocation_map_[reinterpret_cast<UINT64>(locked.get())] = false;
		}
	}

	void CommandPairPool::Initialize(ID3D12Device2* dev, const Weak <DescriptorHandler>& handler)
	{
		if (bool expected = false; 
			m_b_initialized_.compare_exchange_strong(expected, true))
		{
			m_dev_ = dev;
			if (const Strong<DescriptorHandler>& locked = handler.lock()) 
			{
				m_heap_handler_ = locked;
			}
			m_pool_.reserve(size);

			for (size_t i = 0; i < size; ++i)
			{
				const auto& allocated = std::allocate_shared<CommandPair>(m_command_pair_pool_);

				const auto address         = reinterpret_cast<address_value>(allocated.get());
				m_pool_[address]           = allocated;
				m_allocation_map_[address] = false;
			}
		}
	}

	void CommandPairTask::Initialize(ID3D12Device2* dev, const Weak<DescriptorHandler>& heap_handler, const size_t buffer_count)
	{
		m_dev_ = dev;

		for (int i = 0; i < std::size(available_); ++i)
		{
			DX::ThrowIfFailed
					(
					 dev->CreateCommandQueue
					 (
					  &queue_descs[i],
					  IID_PPV_ARGS(m_queue_[i].GetAddressOf())
					 )
					);
		}

		m_buffer_count_ = buffer_count;

		m_fence_nonce_ = std::unique_ptr<uint64_t>(new uint64_t[buffer_count]);
		DX::ThrowIfFailed(dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence_.GetAddressOf())));

		m_pool_.Initialize(dev, heap_handler);
	}

	uint64_t CommandPairTask::GetBufferIndex() const
	{
		return m_buffer_idx_;
	}

	ID3D12CommandQueue* CommandPairTask::GetCommandQueue(const D3D12_COMMAND_LIST_TYPE type) const
	{
		if (m_queue_.size() > type)
		{
			return nullptr;
		}

		return m_queue_[type].Get();
	}

	bool CommandPairTask::IsCommandPairAvailable() const
	{
		if (m_dev_ == nullptr)
		{
			return false;
		}

		return m_command_pair_count_.load() < CFG_MAX_CONCURRENT_COMMAND_LIST;
	}

	Weak<CommandPair> CommandPairTask::Acquire(const D3D12_COMMAND_LIST_TYPE type, const bool heap_allocation, const std::wstring_view debug_name)
	{
		if (m_dev_ == nullptr)
		{
			return {};
		}

		if (m_buffer_idx_ == -1)
		{
			return {};
		}

		std::lock_guard l(m_critical_mutex_);
		m_command_pairs_.push_back(m_pool_.Allocate(type, m_command_ids_, m_buffer_idx_, debug_name, heap_allocation));
		m_command_ids_ += 1;
		m_command_pair_count_.fetch_add(1);

		return m_command_pairs_.back();
	}

	void CommandPairTask::WaitForCommandsCompletion() const
	{
		if (m_dev_ == nullptr)
		{
			return;
		}

		while (true)
		{
			const UINT64 count = m_command_pair_count_.load();

			if (count == 0)
			{
				break;
			}

			m_command_pair_count_.wait(count);
		}
	}

	void CommandPairTask::StopTask()
	{
		std::lock_guard l(m_critical_mutex_);
		m_running_ = false;
		Cleanup();
	}

	void CommandPairTask::StartTask()
	{
		m_running_ = true;

		while (m_running_)
		{
			if (const UINT64 count = m_command_pair_count_.load())
			{
				std::lock_guard lock(m_critical_mutex_);

				if (m_command_pairs_.empty())
				{
					__debugbreak();
					continue;
				}

				if (const auto& queued = m_command_pairs_.front().lock();
					queued && (queued->IsExecuted() || queued->IsDisposed()))
				{
					m_command_pair_count_.fetch_sub(1);
					m_command_pairs_.pop_front();
					m_pool_.Deallocate(queued);
					m_command_pair_count_.notify_all();
				}
				else if (queued && queued->IsReady())
				{
					Execute(queued, false);
					m_command_pair_count_.fetch_sub(1);
					m_command_pairs_.pop_front();
					m_pool_.Deallocate(queued);
					m_command_pair_count_.notify_all();
				}
			}
			else
			{
				m_command_pair_count_.wait(count);
			}
		}

		Cleanup();
	}

	void CommandPairTask::SwapBuffer(const uint32_t next_buffer)
	{
		m_fence_nonce_.get()[next_buffer] = m_fence_nonce_.get()[m_buffer_idx_];
			
		uint64_t nonce = m_fence_nonce_.get()[next_buffer];

		DX::ThrowIfFailed(m_queue_[D3D12_COMMAND_LIST_TYPE_DIRECT]->Signal(m_fence_.Get(), ++nonce));
		WaitForEventCompletion(nonce);

		m_buffer_idx_ = next_buffer;
	}

	void CommandPairTask::Cleanup()
	{
		for (const Weak<CommandPair>& pair : m_command_pairs_)
		{
			if (const Strong<CommandPair> locked = pair.lock())
			{
				locked->HardReset();
				m_pool_.Deallocate(locked);
			}
		}

		for (const ComPtr<ID3D12CommandQueue>& queue : m_queue_)
		{
			queue->Release();
		}

		if (m_fence_event_ != nullptr)
		{
			CloseHandle(m_fence_event_);
		}
	}

	void CommandPairTask::Execute(const Strong<CommandPair>& pair, const bool lock_consuming)
	{
		if (m_dev_ == nullptr)
		{
			return;
		}

		std::lock_guard l(m_critical_mutex_);

		if (lock_consuming)
		{
			std::lock_guard pl(pair->m_critical_mutex_);
			ExecuteImpl(pair);
			return;
		}

		ExecuteImpl(pair);
	}

	void CommandPairTask::Signal(const Strong<CommandPair>& in_pair) const
	{
		if (in_pair->GetAllocator() == nullptr || in_pair->GetList() == nullptr)
		{
			return;
		}

		const auto& fence = m_fence_;
		uint64_t    nonce = m_fence_nonce_.get()[in_pair->GetBufferIndex()];

		DX::ThrowIfFailed(m_queue_[in_pair->GetType()]->Signal(fence.Get(), ++nonce));
		in_pair->m_latest_fence_value_.store(nonce);
	}

	void CommandPairTask::WaitForEventCompletion(const uint64_t fence_value)
	{
		if (m_fence_->GetCompletedValue() < fence_value)
		{
			m_fence_event_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);

			DX::ThrowIfFailed
					(
					 m_fence_->SetEventOnCompletion
					 (
					  fence_value,
					  m_fence_event_
					 )
					);

			WaitForSingleObject(m_fence_event_, INFINITE);
			CloseHandle(m_fence_event_);
			m_fence_event_ = nullptr;
		}
	}

	void CommandPairTask::ExecuteImpl(const Strong<CommandPair>& pair)
	{
		if (pair->IsExecuted())
		{
			return;
		}

		DX::ThrowIfFailed(pair->GetList()->Close());
		const std::vector<ID3D12CommandList*> lists(1, pair->GetList());
		m_queue_[pair->GetType()]->ExecuteCommandLists(1, lists.data());

		Signal(pair);
		WaitForEventCompletion(pair->GetLatestFenceValue());

		if (pair->m_post_execute_function_)
		{
			pair->m_post_execute_function_();
		}

		pair->m_b_ready_    = false;
		pair->m_b_executed_ = true;
	}
}
