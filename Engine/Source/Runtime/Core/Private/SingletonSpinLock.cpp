#include "SingletonSpinLock.h"
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
	m_spin_locks_.emplace(m_nonce_, make_managed_shared<std::atomic<bool>>());
	const size_t return_value = m_nonce_;
	m_nonce_ = m_nonce_ + 1 % (size_t)-2;

	SelfUnlock();

	return SpinLockTicket(return_value);
}

Engine::SpinLockToken Engine::SingletonSpinLock::Lock(const SpinLockTicket& ticket)
{
	Engine::Strong<std::atomic<bool>> lock_ptr;
	SelfLock();
	if ( !m_spin_locks_.contains( ticket.m_idx_ ) )
	{
		SelfUnlock();
		return SpinLockToken{ (size_t)-1 };
	}
	lock_ptr = m_spin_locks_[ticket.m_idx_];
	SelfUnlock();

	while ( lock_ptr->exchange( true ) )
	{ }
	return SpinLockToken{ ticket.m_idx_ };
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
	Engine::Strong<std::atomic<bool>> lock_ptr;
	SelfLock();
	if ( !m_spin_locks_.contains( idx ) )
	{
		SelfUnlock();
		return;
	}
	lock_ptr = m_spin_locks_[idx];
	SelfUnlock();

	if ( lock_ptr )
		lock_ptr->store( false );
}

void Engine::SingletonSpinLock::SelfLock()
{
	while ( m_critical_lock_.exchange( true ) )
	{ }
}

void Engine::SingletonSpinLock::SelfUnlock()
{
	m_critical_lock_.store( false );
}
