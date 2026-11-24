#pragma once
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "ChangeSignal.h"

namespace Engine
{
    // Collects change events between two Drain() calls. Listens only while it exists. Drain() runs on the thread
    // that makes the changes (the main thread). Events are appended as they come (an entity can
    // appear several times); MergeChanges() folds them per entity when a consumer needs that.
    class ENGINE_CORE_API ChangeCollector
    {
    public:
        ChangeCollector();
        ~ChangeCollector();

        ChangeCollector( const ChangeCollector& )            = delete;
        ChangeCollector& operator=( const ChangeCollector& ) = delete;

        // Moves the events collected since the previous drain into `events` (its contents are replaced, its
        // capacity is kept for the next collection) and returns the new epoch. A consumer keeping per-frame copies
        // (frames in flight) tags them with the epoch.
        uint64_t Drain( std::vector<ChangeEvent>& events );

    private:
        void Add( const ChangeEvent& event );

        std::mutex               m_mutex_;
        std::vector<ChangeEvent> m_pending_;
        uint64_t                 m_epoch_ = 0;
        DelegateHandle           m_handle_;
    };

    // Events folded per entity.
    struct ENGINE_CORE_API MergedChanges
    {
        // eChange bits per object (object-level events) and per component.
        std::unordered_map<GlobalEntityID, uint16_t> objects;
        std::unordered_map<GlobalEntityID, uint16_t> components;
        // Owner object of each component that changed.
        std::unordered_map<GlobalEntityID, GlobalEntityID> component_owners;
        // Scenes whose contents were replaced as a whole.
        std::unordered_map<GlobalEntityID, uint16_t> scenes;

        [[nodiscard]] bool empty() const noexcept
        {
            return objects.empty() && components.empty() && scenes.empty();
        }
    };

    ENGINE_CORE_API MergedChanges MergeChanges( const std::vector<ChangeEvent>& events );
}
