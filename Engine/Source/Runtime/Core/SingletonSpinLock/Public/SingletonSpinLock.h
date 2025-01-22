#pragma once
#include "Singleton.h"
#include "Allocator/Public/Allocator.h"

#include "SingletonSpinLock.generated.h"


namespace Engine
{
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

		size_t Register();
		SpinLockToken Lock(const size_t idx);

	private:
		SingletonSpinLock() = default;
		~SingletonSpinLock() override;
		friend struct SpinLockToken;
		friend struct SingletonDeleter;

		void Unlock(const size_t idx);

		void SelfLock();
		void SelfUnlock();

		std::atomic<bool>                                     m_critical_lock_;
		size_t                                                m_nonce_;
		std::unordered_map<size_t, Strong<std::atomic<bool>>> m_spin_locks_;
		u_align_allocator<std::atomic<bool>>                  m_allocator_;
	};

	struct SpinLockToken
	{
		~SpinLockToken()
		{
			if (IsValid())
			{
				Engine::SingletonSpinLock::GetInstance().Unlock(m_idx_);	
			}
		}

		SpinLockToken(const SpinLockToken&) = delete;
		SpinLockToken& operator=(const SpinLockToken&) = delete;

		explicit SpinLockToken(size_t idx) :
		m_idx_(idx) {}

		bool IsValid() const
		{
			return m_idx_ != -1;
		}

	private:
		size_t m_idx_ = -1;
	};
}
