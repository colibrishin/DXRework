#pragma once
#include <boost/pool/pool_alloc.hpp>
#include <wrl/client.h>

namespace Engine
{
	struct CommandPair final
	{
	public:
		explicit CommandPair(
			eCommandTypes type, UINT64 ID, UINT64 buffer_idx, const std::wstring& debug_name
		);

		CommandPair(const CommandPair& other)            = delete;
		CommandPair& operator=(const CommandPair& other) = delete;

		void SetDisposed();
		void HardReset();
		void SoftReset();

		// post_execution is called after the command list is executed.
		// thread-safety of post_execution inside values should be guaranteed by the caller.
		void               FlagReady(const std::function<void()>& post_execution = {});
		[[nodiscard]] bool IsReady();
		[[nodiscard]] bool IsExecuted();
		[[nodiscard]] bool IsDisposed();
		[[nodiscard]] ID3D12GraphicsCommandList1* GetList() const;
		[[nodiscard]] ID3D12GraphicsCommandList4* GetList4() const;
		[[nodiscard]] eCommandTypes               GetType() const;
		[[nodiscard]] UINT64                      GetLatestFenceValue() const;
		[[nodiscard]] UINT64                      GetBufferIndex() const;
		[[nodiscard]] UINT64                      GetID() const;

		void Execute(bool lock_consuming);

	private:
		friend class Manager::Graphics::D3Device;
		template <size_t>
		friend struct CommandPairPool;

		template< class T, class A >
		friend typename boost::detail::sp_if_not_array< T >::type boost::allocate_shared_noinit(A const& a);

		CommandPair() = default;

		[[nodiscard]] UINT64 Signal(eCommandTypes type) const;
		void                 ExecuteImpl();

		UINT64                m_command_id_;
		UINT64                m_buffer_idx_;
		std::wstring          m_debug_name_;
		std::atomic<UINT64>   m_latest_fence_value_;
		std::function<void()> m_post_execute_function_;

		bool       m_b_executed_;
		bool       m_b_ready_;
		bool       m_b_disposed_;
		std::mutex m_critical_mutex_;

		eCommandTypes                      m_type_;
		ComPtr<ID3D12CommandAllocator>     m_allocator_;
		ComPtr<ID3D12GraphicsCommandList1> m_list_;
		ComPtr<ID3D12GraphicsCommandList4> m_list4_;
	};

	template <size_t Size = 256>
	struct CommandPairPool
	{
		using address_value = UINT64;

		CommandPairPool()
		{
			initialize();
		}

		CommandPairPool(const CommandPairPool&)
		{
			initialize();
		}

		Weak<CommandPair> allocate(const eCommandTypes type, const UINT64 id, const UINT64 buffer_idx, const std::wstring& debug_name)
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

				DX::ThrowIfFailed(m_pool_[available]->m_list_->SetName(debug_name.c_str()));
			}
			else
			{
				new(m_pool_[available].get()) CommandPair(type, id, buffer_idx, debug_name);
			}
			
			return m_pool_[available];
		}

		void deallocate(const Weak<CommandPair>& pointer)
		{
			std::lock_guard lock(m_mutex_);

			if (const auto& locked = pointer.lock())
			{
				locked->HardReset();
				m_allocation_map_[reinterpret_cast<UINT64>(locked.get())] = false;
			}
		}


	private:
		void initialize()
		{
			if (bool expected = false; 
				m_b_initialized_.compare_exchange_strong(expected, true))
			{
				m_pool_.reserve(Size);

				for (size_t i = 0; i < Size; ++i)
				{
					const auto& allocated = boost::allocate_shared_noinit<CommandPair>(m_command_pair_pool_);

					const address_value address = reinterpret_cast<UINT64>(allocated.get());

					m_pool_[address] = allocated;
					m_allocation_map_[address] = false;
				}
			}
		}

		std::mutex m_mutex_;
		std::atomic<bool> m_b_initialized_;
		fast_pool_unordered_map<address_value, Strong<CommandPair>> m_pool_;
		std::unordered_map<address_value, bool> m_allocation_map_;
		boost::pool_allocator<CommandPair> m_command_pair_pool_;

	};

	inline static CommandPairPool s_command_pair_pool{};
}
