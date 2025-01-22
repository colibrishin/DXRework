#include "SingletonSpinLock/Public/SingletonSpinLock.h"
#include "SingletonSpinLock.generated.h"

Engine::SpinLockTicket Engine::SingletonSpinLock::Register()
{
	SelfLock();

	std::atomic<bool> new_lock;
	while (true) 
	{
		if (m_spin_locks_.contains(m_nonce_)) 
		{
			m_nonce_ = m_nonce_ + 1 % (size_t)-2;
		}
		else 
		{
			break;
		}
	}
	m_spin_locks_.emplace(m_nonce_, boost::allocate_shared<std::atomic<bool>>(m_allocator_));
	const size_t return_value = m_nonce_;
	m_nonce_ = m_nonce_ + 1 % (size_t)-2;

	SelfUnlock();

	return SpinLockTicket(return_value);
}

Engine::SpinLockToken Engine::SingletonSpinLock::Lock(const SpinLockTicket& ticket)
{
	SelfLock();
	if (!m_spin_locks_.contains(ticket.idx))
	{
		return SpinLockToken{(size_t)-1};
	}
	SelfUnlock();
	

	while (true)
	{
		bool success = false;
		SelfLock();
		bool expected = false;
		success = m_spin_locks_[ticket.idx]->compare_exchange_strong(expected, true);
		SelfUnlock();

		if (success) return SpinLockToken{ticket.idx};
	}
}

Engine::SingletonSpinLock::~SingletonSpinLock() {}

void Engine::SingletonSpinLock::Unregister(const size_t idx)
{
	SelfLock();
	if (m_spin_locks_.contains(idx)) 
	{
		m_spin_locks_[idx] = {};
	}
	SelfUnlock();
}

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
