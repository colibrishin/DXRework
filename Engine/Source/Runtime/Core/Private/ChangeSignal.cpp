#include "ChangeSignal.h"

#include "Actor.h"
#include "Component.h"
#include "ObjectBase.h"
#include "Scene.h"

#include <atomic>
#include <cstdlib>
#include <cstring>

namespace Engine
{
    namespace
    {
        std::atomic<uint64_t> g_epoch{ 1 };

#if WITH_EDITOR || WITH_DEBUG
        // KOLIBRI_DIRTY_MUTATE: comma-separated call sites whose events are dropped, read once.
        const std::string g_mutated_sites = "," + GetDiagSwitch( "KOLIBRI_DIRTY_MUTATE" ).value_or( "" ) + ",";
#endif
    }

    ConcurrentDelegate<ChangeEvent>& OnChange()
    {
        // Never destroyed: a change sent during static initialization or late in shutdown still finds the channel.
        static auto* const channel = new ConcurrentDelegate<ChangeEvent>();
        return *channel;
    }

    bool ChangeSignalActive() noexcept
    {
        return OnChange().ListenerCount() != 0;
    }

    void EmitChange( const char* site, const ChangeEvent& event )
    {
#if WITH_EDITOR || WITH_DEBUG
        if ( g_mutated_sites.size() > 2 && g_mutated_sites.find( std::string( "," ) + site + "," ) != std::string::npos )
        {
            return;
        }
#else
        ( void )site;
#endif
        OnChange().Broadcast( event );
    }

    uint64_t ChangeEpoch() noexcept
    {
        return g_epoch.load( std::memory_order_relaxed );
    }

    void AdvanceChangeEpoch() noexcept
    {
        g_epoch.fetch_add( 1, std::memory_order_relaxed );
    }

    void SignalActor( const char* site, const Abstracts::Actor& actor, const eChange kind )
    {
        if ( !ChangeSignalActive() )
        {
            return;
        }
        const Strong<Scene> scene = actor.GetScene().lock();
        EmitChange( site, { scene ? scene->GetID() : 0, actor.GetID(), 0, kind } );
    }

    void SignalComponent( const char* site, const Abstracts::Component& component, const eChange kind )
    {
        if ( !ChangeSignalActive() )
        {
            return;
        }
        GlobalEntityID scene_id = 0;
        GlobalEntityID owner_id = 0;
        if ( const Strong<Abstracts::ObjectBase> owner = component.GetOwner().lock() )
        {
            owner_id = owner->GetID();
            if ( const Strong<Scene> scene = owner->GetScene().lock() )
            {
                scene_id = scene->GetID();
            }
        }
        EmitChange( site, { scene_id, owner_id, component.GetID(), kind } );
    }

    void SignalScene( const char* site, const Scene& scene )
    {
        if ( ChangeSignalActive() )
        {
            EmitChange( site, { scene.GetID(), 0, 0, eChange::SceneReset } );
        }
    }

    std::optional<std::string> GetDiagSwitch( const char* name )
    {
#if WITH_EDITOR || WITH_DEBUG
        char*  value  = nullptr;
        size_t length = 0;
        if ( _dupenv_s( &value, &length, name ) == 0 && value )
        {
            std::string result( value );
            std::free( value );
            return result;
        }
#else
        ( void )name;
#endif
        return std::nullopt;
    }
}
