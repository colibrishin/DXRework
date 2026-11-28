#pragma once
#include <cstdint>
#include <optional>
#include <string>

#include "Delegation.hpp"
#include "TypeLibrary.h"

// Change signal: every change of an input the renderer reads (transform, active state, hierarchy, layer, shape,
// material, ...) is announced on one channel, so a consumer can process only what changed. Nothing is built or
// sent while nobody listens: call sites check ChangeSignalActive() before they gather the event.

namespace Engine
{
    class Scene;

    namespace Abstracts
    {
        class Actor;
        class Component;
    }

    // What changed. The values are bits so a consumer can merge the kinds per entity.
    enum class eChange : uint16_t
    {
        Transform  = 1 << 0, // the object's transform (its descendants' world matrices follow)
        Added      = 1 << 1, // object or component added to the scene
        Removed    = 1 << 2, // object or component removed from the scene
        Active     = 1 << 3, // active flag (an object's is inherited by its descendants)
        Owner      = 1 << 4, // component moved to another object
        Reparented = 1 << 5, // object attached to or detached from a parent
        Layer      = 1 << 6, // object moved to another layer
        Modified   = 1 << 7, // another input of the component (shape, material, texture, ...)
        SceneReset = 1 << 8, // the whole scene was replaced (activation, deserialization, synchronization)
    };

    constexpr uint16_t operator|( const eChange a, const eChange b ) noexcept
    {
        return static_cast<uint16_t>( a ) | static_cast<uint16_t>( b );
    }

    constexpr uint16_t operator|( const uint16_t a, const eChange b ) noexcept
    {
        return a | static_cast<uint16_t>( b );
    }

    struct ChangeEvent
    {
        GlobalEntityID scene     = 0; // 0 when the source does not look it up (transform events)
        GlobalEntityID object    = 0;
        GlobalEntityID component = 0; // 0 when the change is about the object itself
        eChange        kind      = eChange::Modified;
    };

    // The channel. Events are sent by the thread that makes the change; today every render input changes on the
    // main thread, and consumers drain there too (resource revisions are plain counters for the same reason).
    // Listeners can run while scene locks are held: they must only record the event, never call into Scene or
    // ObjectBase.
    ENGINE_CORE_API ConcurrentDelegate<ChangeEvent>& OnChange();

    // True while the channel has a listener; call sites skip gathering the event otherwise.
    ENGINE_CORE_API bool ChangeSignalActive() noexcept;

    // Sends an event. `site` names the call site for the missing-signal check's mutation switch
    // (KOLIBRI_DIRTY_MUTATE=<site,...> drops those sites' events in Editor/Debug builds).
    ENGINE_CORE_API void EmitChange( const char* site, const ChangeEvent& event );

    // Advanced by every ChangeCollector::Drain. A source that changes many times between two drains (a transform
    // moved by several setters) sends one event per epoch. Starts at 1.
    ENGINE_CORE_API uint64_t ChangeEpoch() noexcept;
    ENGINE_CORE_API void     AdvanceChangeEpoch() noexcept;

    // A member that copies do not take over: copy construction gives a default value and copy assignment keeps
    // the target's value. For per-instance bookkeeping in copyable components (e.g. Rigidbody's Transform copy).
    template <typename T>
    struct NotCopied
    {
        T value{};

        NotCopied() = default;
        NotCopied( const NotCopied& ) {}
        NotCopied& operator=( const NotCopied& ) { return *this; }
    };

    // Send an event about an object (or any actor), a component (with its owner) or a whole scene. They do nothing
    // while nobody listens.
    ENGINE_CORE_API void SignalActor( const char* site, const Abstracts::Actor& actor, eChange kind );
    ENGINE_CORE_API void SignalComponent( const char* site, const Abstracts::Component& component, eChange kind );
    ENGINE_CORE_API void SignalScene( const char* site, const Scene& scene );

    // Diagnostic switches read from the environment in Editor and Debug builds; always empty in others.
    ENGINE_CORE_API std::optional<std::string> GetDiagSwitch( const char* name );
}
