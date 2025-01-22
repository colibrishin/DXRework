#include "SingletonSpinLock/Public/SingletonSpinLock.h"
#include "SingletonSpinLock.generated.h"

size_t Engine::SingletonSpinLock::Register()
{
	SelfLock();

	std::atomic<bool> new_lock;
	m_spin_locks_.emplace(m_nonce_, boost::allocate_shared<std::atomic<bool>>(m_allocator_));
	const size_t return_value = m_nonce_;
	m_nonce_ = m_nonce_ + 1 % (size_t)-2;

	SelfUnlock();

	return return_value;
}

Engine::SpinLockToken Engine::SingletonSpinLock::Lock(const size_t idx)
{
	SelfLock();
	if (!m_spin_locks_.contains(idx))
	{
		return SpinLockToken{(size_t)-1};
	}
	SelfUnlock();
	

	while (true)
	{
		bool success = false;
		SelfLock();
		bool expected = false;
		success = m_spin_locks_[idx]->compare_exchange_strong(expected, true);
		SelfUnlock();

		if (success) return SpinLockToken{idx};
	}
}

Engine::SingletonSpinLock::~SingletonSpinLock() {}

void Engine::SingletonSpinLock::Unlock(const size_t idx)
{
	SelfLock();
	if (!m_spin_locks_.contains(idx))
	{
		return;
	}
	SelfUnlock();
	

	while (true)
	{
		bool success = false;
		SelfLock();
		bool expected = true;
		success = m_spin_locks_[idx]->compare_exchange_strong(expected, false);
		SelfUnlock();

		if (success) return;
	}
}

void Engine::SingletonSpinLock::SelfLock()
{
	{
		bool expected = false;
		while (!m_critical_lock_.compare_exchange_strong(expected, true)) {}
	}
}

void Engine::SingletonSpinLock::SelfUnlock()
{
	{
		bool expected = true;
		while (!m_critical_lock_.compare_exchange_strong(expected, false)) {}
	}
}
