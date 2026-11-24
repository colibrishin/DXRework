#include "ChangeCollector.h"

namespace Engine
{
    ChangeCollector::ChangeCollector()
    {
        m_handle_ = OnChange().ListenScoped( [ this ]( const ChangeEvent& event ) { Add( event ); } );
        // A source that already sent in the current epoch (to an earlier collector) would stay silent otherwise.
        AdvanceChangeEpoch();
    }

    ChangeCollector::~ChangeCollector()
    {
        // Waits for events being added on other threads before the members go away.
        m_handle_.Reset();
        AdvanceChangeEpoch();
    }

    void ChangeCollector::Add( const ChangeEvent& event )
    {
        std::lock_guard lock( m_mutex_ );
        m_pending_.push_back( event );
    }

    uint64_t ChangeCollector::Drain( std::vector<ChangeEvent>& events )
    {
        events.clear();
        std::lock_guard lock( m_mutex_ );
        // Advance before the hand-over: a source stamped with the new epoch adds its event after the swap (Add waits
        // for the lock), so it lands in the next batch.
        AdvanceChangeEpoch();
        std::swap( events, m_pending_ );
        return ++m_epoch_;
    }

    MergedChanges MergeChanges( const std::vector<ChangeEvent>& events )
    {
        MergedChanges merged;
        for ( const ChangeEvent& event : events )
        {
            const auto bits = static_cast<uint16_t>( event.kind );
            if ( event.kind == eChange::SceneReset )
            {
                merged.scenes[ event.scene ] |= bits;
            }
            else if ( event.component != 0 )
            {
                merged.components[ event.component ] |= bits;
                merged.component_owners[ event.component ] = event.object;
            }
            else
            {
                merged.objects[ event.object ] |= bits;
            }
        }
        return merged;
    }
}
