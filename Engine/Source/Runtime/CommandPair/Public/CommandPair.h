#pragma once
#include <boost/pool/pool_alloc.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>

#include <array>
#include <atomic>
#include <deque>
#include <mutex>

#include <directx/d3d12.h>
#include <wrl/client.h>

#include "Source/Runtime/Core/Allocator/Public/Allocator.h"
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

namespace Engine
{
	struct CommandPairPool;
	struct CommandPairTask;

	struct COMMANDPAIR_API CommandPair final
	{
	public:
		explicit CommandPair
		(
			CommandPairPool* pool,
			const D3D12_COMMAND_LIST_TYPE type,
			const UINT64 ID,
			const UINT64 buffer_idx,
			const std::wstring& debug_name
		);

		CommandPair(const CommandPair& other)            = delete;
		CommandPair& operator=(const CommandPair& other) = delete;

		void SetDisposed();
		void HardReset();
		void SoftReset();

		// post_execution is called after the command list is executed.
		// thread-safety of post_execution inside values should be guaranteed by the caller.
		void               FlagReady(const std::function<void()>& post_execution = {});
		void               Execute() const;
		[[nodiscard]] bool IsReady();
		[[nodiscard]] bool IsExecuted();
		[[nodiscard]] bool IsDisposed();

		[[nodiscard]] ID3D12GraphicsCommandList1* GetList() const;
		[[nodiscard]] ID3D12GraphicsCommandList4* GetList4() const;
		[[nodiscard]] ID3D12CommandAllocator*     GetAllocator() const;

		[[nodiscard]] D3D12_COMMAND_LIST_TYPE     GetType() const;
		[[nodiscard]] UINT64                      GetLatestFenceValue() const;
		[[nodiscard]] UINT64                      GetBufferIndex() const;
		[[nodiscard]] UINT64                      GetID() const;

	private:
		friend class Managers::D3Device;
		friend struct CommandPairPool;
		friend struct CommandPairTask;

		template< class T, class A >
		friend typename boost::detail::sp_if_not_array< T >::type boost::allocate_shared_noinit(A const& a);

		CommandPair() = default;

		CommandPairPool* m_pool_{};

		UINT64       m_command_id_{};
		UINT64       m_buffer_idx_{};
		std::wstring m_debug_name_;

		std::atomic<uint64_t>   m_latest_fence_value_;
		std::function<void()> m_post_execute_function_;

		std::atomic<bool>       m_b_executed_{};
		std::atomic<bool>       m_b_ready_{};
		std::atomic<bool>       m_b_disposed_{};
		std::mutex m_critical_mutex_;

		D3D12_COMMAND_LIST_TYPE            m_type_ = D3D12_COMMAND_LIST_TYPE_NONE;
		ComPtr<ID3D12CommandAllocator>     m_allocator_{};
		ComPtr<ID3D12GraphicsCommandList1> m_list_{};
		ComPtr<ID3D12GraphicsCommandList4> m_list4_{};
	};

	struct COMMANDPAIR_API CommandPairPool final
	{
		using address_value = UINT64;
		static constexpr size_t size = 256;

		CommandPairPool() = default;

		Weak<CommandPair> Allocate(const D3D12_COMMAND_LIST_TYPE type, const UINT64 id, const UINT64 buffer_idx, const std::wstring& debug_name);
		void Deallocate(const Weak<CommandPair>& pointer);
		void Initialize(ID3D12Device2* dev);

	private:
		friend struct CommandPair;

		std::mutex                m_mutex_;
		std::atomic<bool>         m_b_initialized_;
		D3D12_COMMAND_LIST_TYPE   m_type_ = D3D12_COMMAND_LIST_TYPE_NONE;

		fast_pool_unordered_map<address_value, Strong<CommandPair>> m_pool_{};
		std::unordered_map<address_value, bool>                     m_allocation_map_{};
		boost::pool_allocator<CommandPair>                          m_command_pair_pool_{};

		ComPtr<ID3D12Device2> m_dev_{};
	};

	struct COMMANDPAIR_API CommandPairTask
	{
	private:
		constexpr static D3D12_COMMAND_QUEUE_DESC queue_descs[]
		{
			{
				.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
				.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
				.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
				.NodeMask = 0
			},
			{
				.Type = D3D12_COMMAND_LIST_TYPE_BUNDLE,
				.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
				.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
				.NodeMask = 0
			},
			{
				.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE,
				.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
				.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
				.NodeMask = 0
			},
			{
				.Type = D3D12_COMMAND_LIST_TYPE_COPY,
				.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
				.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
				.NodeMask = 0
			},
		};

		constexpr static D3D12_COMMAND_LIST_TYPE available_[]
		{
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			D3D12_COMMAND_LIST_TYPE_BUNDLE,
			D3D12_COMMAND_LIST_TYPE_COMPUTE,
			D3D12_COMMAND_LIST_TYPE_COPY
		};

	public:
		CommandPairTask() = default;

		void Initialize(ID3D12Device2* dev, const size_t buffer_count);

		[[nodiscard]] uint64_t GetBufferIndex() const;
		[[nodiscard]] ID3D12CommandQueue* GetCommandQueue(const D3D12_COMMAND_LIST_TYPE type) const;
		[[nodiscard]] bool IsCommandPairAvailable() const;
		[[nodiscard]] Weak<CommandPair> Acquire(const D3D12_COMMAND_LIST_TYPE type, const std::wstring& debug_name);

		void WaitForCommandsCompletion() const;
		void StopTask();
		void StartTask();
		void SwapBuffer(const uint32_t next_buffer);

	private:
		friend struct CommandPair;

		void Cleanup();

		void Execute(const Strong<CommandPair>& pair, const bool lock_consuming);
		void Signal(const Strong<CommandPair>& in_pair) const;
		void WaitForEventCompletion(const uint64_t fence_value);
		void ExecuteImpl(const Strong<CommandPair>& pair);

		std::atomic<bool> m_running_;

		CommandPairPool                 m_pool_{};
		std::deque<Weak<CommandPair>>   m_command_pairs_{};

		std::array<ComPtr<ID3D12CommandQueue>, std::size(queue_descs)> m_queue_{};
		ComPtr<ID3D12Device2>                                        m_dev_{};

		uint64_t              m_command_ids_{};
		std::mutex            m_critical_mutex_;
		std::atomic<uint64_t> m_command_pair_count_;

		uint64_t                  m_buffer_count_{};
		uint64_t                  m_buffer_idx_{};
		ComPtr<ID3D12Fence>       m_fence_{};
		HANDLE                    m_fence_event_{};
		std::unique_ptr<uint64_t> m_fence_nonce_{};
	};
}
