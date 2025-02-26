#pragma once
#include "Singleton.h"
#include "Allocator.h"

#include "SingletonSpinLock.generated.h"


namespace Engine
{
	struct SpinLockTicket;
	struct SpinLockToken;

	ECLASS()
	class ENGINE_CORE_API SingletonSpinLock : public Abstracts::Singleton<SingletonSpinLock>
	{
		GENERATE_BODY
	public:
		explicit SingletonSpinLock(SINGLETON_LOCK_TOKEN) :
		m_nonce_(0) {}

		void PreUpdate(const float dt) override {}
		void Update(const float dt) override {}
		void PostUpdate(const float dt) override {}
		void FixedUpdate(const float dt) override {}
		void PreRender(const float dt) override {}
		void Render(const float dt) override {}
		void PostRender(const float dt) override {}
		void Initialize() override {}

		SpinLockTicket Register();
		SpinLockToken Lock(const SpinLockTicket& idx);

	private:
		SingletonSpinLock() = default;
		~SingletonSpinLock() override;
		friend struct SpinLockToken;
		friend struct SpinLockTicket;
		friend struct SingletonDeleter;

		void Unregister(const size_t idx);
		void Unlock(const size_t idx);

		void SelfLock();
		void SelfUnlock();

		std::atomic<bool>                                     m_critical_lock_;
		size_t                                                m_nonce_ = 0;
		std::unordered_map<size_t, Strong<std::atomic<bool>>> m_spin_locks_;
		u_fast_pool_allocator_single<std::atomic<bool>>       m_allocator_;
	};

	struct SpinLockTicket
	{
		~SpinLockTicket()
		{
			if (m_idx_ != -1) 
			{
				SingletonSpinLock::GetInstance().Unregister(m_idx_);
			}
		}

	    SpinLockTicket(const SpinLockTicket&) = delete;
		SpinLockTicket& operator=(const SpinLockTicket&) = delete;

		SpinLockTicket(SpinLockTicket&& other) noexcept
        {
            operator=( std::move( other ) );
		}

		SpinLockTicket& operator=(SpinLockTicket&& other) noexcept
        {
		    m_idx_ = other.m_idx_;
            other.m_idx_ = -1;
			return *this;
		}

	private:
        explicit SpinLockTicket(const size_t idx) : m_idx_(idx) {}
		friend class SingletonSpinLock;
		size_t m_idx_;
	};

	struct SpinLockToken
	{
		~SpinLockToken()
		{
			Release();
		}

		SpinLockToken(const SpinLockToken&) = delete;
		SpinLockToken& operator=(const SpinLockToken&) = delete;

		SpinLockToken(SpinLockToken&& other) noexcept
        {
            m_idx_ = other.m_idx_;
            other.m_idx_ = -1;
		}

		SpinLockToken& operator=(SpinLockToken&& other) noexcept
        {
		    m_idx_ = other.m_idx_;
            other.m_idx_ = -1;
			return *this;
		}

		explicit SpinLockToken(size_t idx) :
		m_idx_(idx) {}

		void Release()
		{
			if (IsValid())
			{
				SingletonSpinLock::GetInstance().Unlock(m_idx_);
				m_idx_ = -1;
			}
		}
		
		[[nodiscard]] bool IsValid() const
		{
			return m_idx_ != -1;
		}

	private:
		size_t m_idx_ = -1;
	};
}
