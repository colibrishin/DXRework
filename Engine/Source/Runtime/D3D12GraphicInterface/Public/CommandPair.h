#pragma once
#include <boost/pool/pool_alloc.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>

#include <array>
#include <atomic>
#include <deque>
#include <mutex>

#include <directx/d3d12.h>
#include <wrl/client.h>

#include "DescriptorHandler.hpp"

#include "Allocator.h"
#include "TypeLibrary.h"
#include "DescriptorPtrImpl.h"
#include "FenceRetireQueue.h"
#include "FrameRelease.h"

namespace Engine
{
	struct CommandPairPool;
	struct CommandPairTask;

	struct ENGINE_D3D12GRAPHICINTERFACE_API CommandPair final : public ICommandList
	{
	public:
		explicit CommandPair
		(
			CommandPairPool* pool,
			const D3D12_COMMAND_LIST_TYPE type,
			const UINT64 ID,
			const UINT64 buffer_idx,
			const std::wstring_view debug_name,
			DescriptorPtr&& heap
		);

		~CommandPair() override;
		CommandPair(const CommandPair& other)            = delete;
		CommandPair& operator=(const CommandPair& other) = delete;

		void SetDisposed();
		void HardReset();
		void SoftReset();

		// post_execution is called after the command list is executed.
		// thread-safety of post_execution inside values should be guaranteed by the caller.
		void               FlagReady(const std::function<void()>& post_execution = {}) override;
		void               Execute() override;
		// Flags the list ready and waits until it is handed to the GPU queue (not until it finished); returns its
		// fence value. Work queued after this on the same queue runs after the list.
		uint64_t           Submit();
		// Runs on the command thread right after the list is handed to the queue (e.g. a fence signal that must
		// follow it). Set before FlagReady.
		void               SetSubmitHook(std::function<void()>&& hook);
		void               DeferRelease(std::function<void()>&& holder) override;
		[[nodiscard]] bool IsReady();
		[[nodiscard]] bool IsSubmitted(); // handed to the GPU queue
		[[nodiscard]] bool IsExecuted();  // finished on the GPU
		[[nodiscard]] bool IsDisposed();

		[[nodiscard]] ID3D12GraphicsCommandList1* GetList() const;
		[[nodiscard]] ID3D12GraphicsCommandList4* GetList4() const;
		[[nodiscard]] ID3D12CommandAllocator*     GetAllocator() const;

		[[nodiscard]] D3D12_COMMAND_LIST_TYPE     GetType() const;
		[[nodiscard]] UINT64                      GetLatestFenceValue() const;
		[[nodiscard]] UINT64                      GetBufferIndex() const;
		[[nodiscard]] UINT64                      GetID() const;

	private:
        void Close() override;

		friend struct CommandPairPool;
		friend struct CommandPairTask;
        friend struct ConstructorAccess;

		CommandPair() = default;

		CommandPairPool* m_pool_{};

		UINT64       m_command_id_{};
		UINT64       m_buffer_idx_{};
		std::wstring m_debug_name_;

		std::atomic<uint64_t>   m_latest_fence_value_;
		std::function<void()> m_post_execute_function_;
		std::function<void()> m_submit_hook_;
		std::vector<std::function<void()>> m_deferred_release_;
		std::mutex                         m_deferred_mutex_; // separate: m_critical_mutex_ is held while waiting for execution

		std::atomic<bool>       m_b_submitted_{};
		std::atomic<bool>       m_b_executed_{};
		std::atomic<bool>       m_b_ready_{};
		std::atomic<bool>       m_b_disposed_{};
		std::mutex m_critical_mutex_;

		D3D12_COMMAND_LIST_TYPE            m_type_ = D3D12_COMMAND_LIST_TYPE_NONE;
		ComPtr<ID3D12CommandAllocator>     m_allocator_{};
		ComPtr<ID3D12GraphicsCommandList1> m_list_{};
		ComPtr<ID3D12GraphicsCommandList4> m_list4_{};
		DescriptorPtr					   m_assigned_heap_{};
	};

	struct ENGINE_D3D12GRAPHICINTERFACE_API CommandPairPool final
	{
		using address_value = UINT64;
		static constexpr size_t size = 256;

		CommandPairPool() = default;

		Weak<CommandPair> Allocate(const D3D12_COMMAND_LIST_TYPE type, 
			const UINT64 id, 
			const UINT64 buffer_idx, 
			const std::wstring_view debug_name,
			const bool heap_allocation);
		void Deallocate(const Weak<CommandPair>& pointer);
		[[nodiscard]] bool HasFree();
		void Initialize(ID3D12Device2* dev, const Weak<DescriptorHandlerBase>& handler, CommandPairTask* task);

	private:
        friend struct CommandPair;
        friend struct CommandPairTask;

        void Cleanup();

		std::mutex                m_mutex_;
		CommandPairTask*          m_task_{};
		std::atomic<bool>         m_b_initialized_;
		D3D12_COMMAND_LIST_TYPE   m_type_ = D3D12_COMMAND_LIST_TYPE_NONE;

		fast_pool_unordered_map<address_value, Strong<CommandPair>> m_pool_{};
		std::unordered_map<address_value, bool>                     m_allocation_map_{};
		Strong<DescriptorHandlerBase>                               m_heap_handler_{};

		ComPtr<ID3D12Device2> m_dev_{};
	};

	struct ENGINE_D3D12GRAPHICINTERFACE_API CommandPairTask
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

		constexpr static int64_t s_conversion[]
		{
			0,
			-1,
			1,
			2,
		};

		constexpr static D3D12_COMMAND_LIST_TYPE available_[]
		{
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			D3D12_COMMAND_LIST_TYPE_COMPUTE,
			D3D12_COMMAND_LIST_TYPE_COPY
		};

	public:
		CommandPairTask() = default;
        ~CommandPairTask();

		void Initialize(ID3D12Device2* dev, const Weak<DescriptorHandlerBase>& heap_handler, const size_t buffer_count);

		[[nodiscard]] uint64_t GetBufferIndex() const;
		[[nodiscard]] ID3D12CommandQueue* GetCommandQueue(const D3D12_COMMAND_LIST_TYPE type) const;
		[[nodiscard]] bool IsCommandPairAvailable() const;
		[[nodiscard]] Weak<CommandPair> Acquire(const D3D12_COMMAND_LIST_TYPE type, const bool heap_allocation, const std::wstring_view debug_name);

		void WaitForCommandsCompletion() const;
		void MarkRunning() { m_running_ = true; }
		void StopTask();
		void StartTask();
        void Cleanup();
		// frame_fence: the fence value of the frame's last list (Present's).
		void SwapBuffer(const uint32_t next_buffer, const uint64_t frame_fence);
		// ReleaseAfterFrame (FrameRelease.h): collected per frame, handed to m_frame_retire_ with the frame's fence.
		void ReleaseAfterFrame(std::function<void()>&& holder);
		void EndFrame(const uint64_t frame_fence);
		[[nodiscard]] uint64_t RecordingFrame() const;
		[[nodiscard]] bool     FrameDone(const uint64_t frame) const;

	private:
		friend struct CommandPair;

		void Execute(const Strong<CommandPair>& pair, const bool lock_consuming);
		uint64_t SignalQueue(const D3D12_COMMAND_LIST_TYPE type); // under m_signal_mutex_
		void WaitForEventCompletion(const uint64_t fence_value);
		void ExecuteImpl(const Strong<CommandPair>& pair);
		void Complete(const Strong<CommandPair>& pair); // after the GPU finished the pair's list
		void RetireCompleted();

		std::atomic<bool> m_running_{ false };

		CommandPairPool                 m_pool_{};
		pool_queue<Weak<CommandPair>>   m_command_pairs_{};

		std::array<ComPtr<ID3D12CommandQueue>, std::size(queue_descs)> m_queue_{};
		ComPtr<ID3D12Device2>                                        m_dev_{};

		uint64_t              m_command_ids_{};
		std::mutex            m_critical_mutex_;
		std::atomic<uint64_t> m_command_pair_count_;

		uint64_t                  m_buffer_count_{};
		uint64_t                  m_buffer_idx_{};
		ComPtr<ID3D12Fence>       m_fence_{};

		// One fence value counter for every queue: incremented and signalled under one lock, so values reach the
		// queues in increasing order. Lists of queues other than DIRECT are only submitted with the GPU idle and are
		// waited for, so one queue at a time signals.
		std::mutex                m_signal_mutex_;
		uint64_t                  m_fence_value_{};
		FenceRetireQueue          m_retire_;

		std::mutex                         m_frame_release_mutex_;
		std::vector<std::function<void()>> m_frame_releases_; // of the frame being recorded
		FenceRetireQueue                   m_frame_retire_;   // run by the main thread (SwapBuffer)

		static constexpr size_t                     s_frame_history = 8;
		std::atomic<uint64_t>                       m_frame_index_{};    // frames ended
		std::array<std::atomic<uint64_t>, s_frame_history> m_frame_fences_{}; // fence value per frame index % history
	};
}
