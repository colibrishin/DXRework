#pragma once
#include <cassert>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <utility>

namespace Engine
{
    // Work that must wait until the GPU has passed a fence value: returning a command allocator, running a list's
    // post-execution function, releasing what a list still reads. Values are pushed in non-decreasing order (the
    // caller signals the fence and pushes under one lock). Retire() runs the work whose value has completed, in push
    // order, outside the queue's lock, so the work may push more.
    class FenceRetireQueue
    {
    public:
        void Push( const uint64_t fence_value, std::function<void()>&& work )
        {
            std::lock_guard l( m_mutex_ );
            assert( m_items_.empty() || m_items_.back().first <= fence_value );
            m_items_.emplace_back( fence_value, std::move( work ) );
        }

        // Runs the work of every value <= completed; returns how many ran.
        size_t Retire( const uint64_t completed )
        {
            size_t ran = 0;
            while ( true )
            {
                std::function<void()> work;
                {
                    std::lock_guard l( m_mutex_ );
                    if ( m_items_.empty() || m_items_.front().first > completed )
                    {
                        return ran;
                    }
                    work = std::move( m_items_.front().second );
                    m_items_.pop_front();
                }
                work();
                ++ran;
            }
        }

        // Shutdown, after the GPU went idle.
        size_t RetireAll()
        {
            return Retire( UINT64_MAX );
        }

        // The lowest value still waited for, 0 when nothing is.
        [[nodiscard]] uint64_t OldestPending() const
        {
            std::lock_guard l( m_mutex_ );
            return m_items_.empty() ? 0 : m_items_.front().first;
        }

        [[nodiscard]] bool Empty() const
        {
            std::lock_guard l( m_mutex_ );
            return m_items_.empty();
        }

    private:
        mutable std::mutex                                  m_mutex_;
        std::deque<std::pair<uint64_t, std::function<void()>>> m_items_;
    };
}
