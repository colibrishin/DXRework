#include "DirtyCheck.h"

#if WITH_EDITOR || WITH_DEBUG
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <windows.h>

#include "Animator.h"
#include "AtlasAnimationTexture.h"
#include "ChangeCollector.h"
#include "Material.h"
#include "ModelRenderer.h"
#include "Object.h"
#include "ObjectBase.h"
#include "Scene.h"
#include "SceneManager.h"
#include "Shape.h"
#include "Transform.h"

namespace Engine::DirtyCheck
{
    namespace
    {
        uint64_t HashBytes( const void* data, const size_t size )
        {
            uint64_t    hash  = 1469598103934665603ull;
            const auto* bytes = static_cast<const unsigned char*>( data );
            for ( size_t i = 0; i < size; ++i )
            {
                hash = ( hash ^ bytes[ i ] ) * 1099511628211ull;
            }
            return hash;
        }

        struct MeshState
        {
            const void*              mesh     = nullptr;
            const void*              material = nullptr;
            const void*              shader   = nullptr;
            size_t                   domain   = 0;
            std::vector<const void*> textures;
            const void*              atlas  = nullptr;
            uint64_t                 values = 0; // the material's instance parameters
            uint64_t                 material_revision = 0;
            uint64_t                 shader_revision   = 0;

            // Inputs only; revisions say whether a change was announced.
            bool SameInputs( const MeshState& o ) const
            {
                return mesh == o.mesh && material == o.material && shader == o.shader && domain == o.domain &&
                       textures == o.textures && atlas == o.atlas && values == o.values;
            }
        };

        // Everything ModelRendererRenderInstanceTask reads for one renderer, plus the object's active state and layer.
        struct State
        {
            GlobalEntityID              object = 0;
            std::vector<GlobalEntityID> chain; // parents, nearest first
            bool                        component_active = false;
            bool                        object_active    = false; // the object and all its parents
            LayerSizeType               layer            = 0;
            bool                        has_transform    = false;
            Matrix                      world;
            bool                        has_animator = false;
            GlobalEntityID              animator_id  = 0;
            float                       animator_frame = 0.f; // the primitive applied to the instance
            int                         animator_values[ 7 ]{};
            const void*                 shape        = nullptr;
            uint64_t                    shape_revision = 0;
            const void*                 animations   = nullptr;
            std::vector<MeshState>      meshes;
        };

        bool SameMeshes( const State& a, const State& b )
        {
            if ( a.meshes.size() != b.meshes.size() )
            {
                return false;
            }
            for ( size_t i = 0; i < a.meshes.size(); ++i )
            {
                if ( !a.meshes[ i ].SameInputs( b.meshes[ i ] ) )
                {
                    return false;
                }
            }
            return true;
        }

        // Changed inputs covered by a revision of the resource that holds them. The shape's revision covers its mesh
        // list, animations and the material of each mesh; a mesh's material revision covers its textures, atlas,
        // values and shader; the shader's revision covers its domain.
        bool RevisionMoved( const State& a, const State& b )
        {
            if ( a.shape == b.shape && a.shape_revision != b.shape_revision )
            {
                return true;
            }
            if ( a.shape != b.shape || a.animations != b.animations || a.meshes.size() != b.meshes.size() )
            {
                return false;
            }
            for ( size_t i = 0; i < a.meshes.size(); ++i )
            {
                const MeshState& x = a.meshes[ i ];
                const MeshState& y = b.meshes[ i ];
                if ( x.mesh != y.mesh || x.material != y.material )
                {
                    return false;
                }
                const bool material_moved = x.material_revision != y.material_revision;
                if ( ( x.textures != y.textures || x.atlas != y.atlas || x.values != y.values || x.shader != y.shader ) &&
                     !material_moved )
                {
                    return false;
                }
                if ( x.domain != y.domain && !material_moved &&
                     !( x.shader == y.shader && x.shader_revision != y.shader_revision ) )
                {
                    return false;
                }
            }
            return true;
        }

        using Snapshot = std::unordered_map<GlobalEntityID, State>;

        Snapshot Take( const Scene& scene )
        {
            Snapshot snapshot;
            for ( const Weak<Abstracts::Component>& weak : scene.GetCachedComponentsConcurrent<Components::ModelRenderer>() )
            {
                const Strong<Abstracts::Component> component = weak.lock();
                if ( !component )
                {
                    continue;
                }

                State state;
                state.component_active = component->GetActive();
                if ( const Strong<Abstracts::ObjectBase> object = component->GetOwner().lock() )
                {
                    state.object = object->GetID();
                    state.layer  = object->GetLayer();

                    bool active = object->GetActive();
                    for ( Strong<Abstracts::ObjectBase> parent = object->GetParent().lock(); parent;
                          parent = parent->GetParent().lock() )
                    {
                        state.chain.push_back( parent->GetID() );
                        active = active && parent->GetActive();
                    }
                    state.object_active = active;

                    if ( const Strong<Components::Transform> transform = object->GetComponent<Components::Transform>().lock() )
                    {
                        state.has_transform = true;
                        state.world         = transform->GetWorldMatrix();
                    }
                    if ( const Strong<Components::Animator> animator = object->GetComponent<Components::Animator>().lock() )
                    {
                        const Graphics::AnimatorPrimitive& p = animator->GetPrimitive();
                        state.has_animator                   = true;
                        state.animator_id                    = animator->GetID();
                        state.animator_frame                 = p.currentFrame;
                        const int values[ 7 ] = { p.animationID, p.animationDuration, p.noAnimation, p.atlasX,
                                                  p.atlasY,      p.atlasW,            p.atlasH };
                        std::memcpy( state.animator_values, values, sizeof( values ) );
                    }
                }

                const auto* renderer = static_cast<const Components::ModelRenderer*>( component.get() );
                if ( const Strong<Resources::Shape> shape = renderer->GetShape().lock() )
                {
                    state.shape          = shape.get();
                    state.shape_revision = shape->GetRevision();
                    state.animations     = shape->GetAnimations().lock().get();
                    for ( const auto& [ mesh, weak_material ] : shape->GetMeshes() )
                    {
                        MeshState& out = state.meshes.emplace_back();
                        out.mesh       = mesh.lock().get();
                        if ( const Strong<Resources::Material> material = weak_material.lock() )
                        {
                            out.material          = material.get();
                            out.material_revision = material->GetRevision();
                            if ( const Strong<Resources::ShaderBase> shader = material->GetShader().lock() )
                            {
                                out.shader          = shader.get();
                                out.domain          = shader->GetShaderDomain();
                                out.shader_revision = shader->GetRevision();
                            }
                            for ( const Weak<Resources::Texture>& texture : material->GetTextures() )
                            {
                                out.textures.push_back( texture.lock().get() );
                            }
                            out.atlas = material->GetAtlasTexture().lock().get();

                            Graphics::SBs::InstanceSB values{};
                            material->GetPrimitive().Apply( values );
                            out.values = HashBytes( &values, sizeof( values ) );
                        }
                    }
                }

                snapshot.emplace( component->GetID(), std::move( state ) );
            }
            return snapshot;
        }

        struct Checker
        {
            ChangeCollector          collector;
            std::vector<ChangeEvent> events;
            Snapshot                 previous;
            GlobalEntityID  previous_scene = 0;
            bool            primed         = false;
            uint64_t        frames         = 0;
            // KOLIBRI_DIRTY_WARMUP=N: misses in the first N frames are not judged (the scene is still being built).
            uint64_t        warmup         = std::stoull( GetDiagSwitch( "KOLIBRI_DIRTY_WARMUP" ).value_or( "0" ) );
            uint64_t        changed        = 0;
            std::ofstream   output{ "dirtycheck.txt", std::ios::app };

            void Report( const std::string& line )
            {
                output << line << "\n";
                output.flush();
                OutputDebugStringA( ( line + "\n" ).c_str() );
            }

            void Check( const Scene& scene )
            {
                collector.Drain( events );
                const MergedChanges changes = MergeChanges( events );
                Snapshot                       current = Take( scene );
                const GlobalEntityID           scene_id = scene.GetID();

                std::vector<std::string> misses;
                const auto miss = [ & ]( const GlobalEntityID component, const GlobalEntityID object, const char* what )
                {
                    misses.push_back( "DIRTY MISS frame=" + std::to_string( frames ) + " component=" +
                                      std::to_string( component ) + " object=" + std::to_string( object ) + " " + what );
                };

                if ( primed && scene_id != previous_scene && !changes.scenes.contains( scene_id ) )
                {
                    misses.push_back( "DIRTY MISS frame=" + std::to_string( frames ) + " scene switch without SceneReset" );
                }
                else if ( primed && !changes.scenes.contains( scene_id ) )
                {
                    // A component event also covers its owner object (e.g. a Transform or Animator added to it).
                    std::unordered_map<GlobalEntityID, uint16_t> by_object = changes.objects;
                    for ( const auto& [ component, bits ] : changes.components )
                    {
                        by_object[ changes.component_owners.at( component ) ] |= bits;
                    }
                    const auto object_bits = [ & ]( const GlobalEntityID id ) -> uint16_t
                    {
                        const auto it = by_object.find( id );
                        return it == by_object.end() ? 0 : it->second;
                    };
                    const auto component_bits = [ & ]( const GlobalEntityID id ) -> uint16_t
                    {
                        const auto it = changes.components.find( id );
                        return it == changes.components.end() ? 0 : it->second;
                    };
                    const auto own_bits = [ & ]( const GlobalEntityID id ) -> uint16_t
                    {
                        const auto it = changes.objects.find( id );
                        return it == changes.objects.end() ? 0 : it->second;
                    };
                    // The object or any of its parents, before or now. `bits` reads either the object's own events or
                    // those plus its components' events.
                    const auto in_chain_of = [ & ]( const auto& bits, const State& a, const State& b, const uint16_t mask )
                    {
                        for ( const State* state : { &a, &b } )
                        {
                            if ( bits( state->object ) & mask )
                            {
                                return true;
                            }
                            for ( const GlobalEntityID id : state->chain )
                            {
                                if ( bits( id ) & mask )
                                {
                                    return true;
                                }
                            }
                        }
                        return false;
                    };
                    const auto in_chain = [ & ]( const State& a, const State& b, const uint16_t mask )
                    {
                        return in_chain_of( object_bits, a, b, mask );
                    };

                    // A world matrix follows the transforms on the chain: moved, reparented, or a Transform component
                    // added, removed or moved to another object. Active and Modified events do not cover it.
                    constexpr uint16_t hierarchy = eChange::Transform | eChange::Reparented | eChange::Added |
                                                   eChange::Removed | eChange::Owner;

                    for ( const auto& [ id, now ] : current )
                    {
                        const auto it = previous.find( id );
                        if ( it == previous.end() )
                        {
                            if ( !( component_bits( id ) & ( eChange::Added | eChange::Owner ) ) &&
                                 !( object_bits( now.object ) & static_cast<uint16_t>( eChange::Added ) ) )
                            {
                                miss( id, now.object, "renderer added without Added" );
                            }
                            continue;
                        }

                        const State& before = it->second;
                        bool         any    = false;
                        const auto   check  = [ & ]( const bool differs, const bool covered, const char* what )
                        {
                            if ( differs )
                            {
                                any = true;
                                if ( !covered )
                                {
                                    miss( id, now.object, what );
                                }
                            }
                        };

                        const bool world_differs = now.has_transform != before.has_transform ||
                                                   std::memcmp( &now.world, &before.world, sizeof( Matrix ) ) != 0;
                        check( world_differs, in_chain( now, before, hierarchy ),
                               "world matrix changed without a signal" );
                        check( now.chain != before.chain, in_chain( now, before, static_cast<uint16_t>( eChange::Reparented ) ),
                               "parent changed without Reparented" );
                        check( now.object != before.object, component_bits( id ) & static_cast<uint16_t>( eChange::Owner ),
                               "owner changed without Owner" );
                        check( now.component_active != before.component_active,
                               component_bits( id ) & static_cast<uint16_t>( eChange::Active ),
                               "component active changed without Active" );
                        check( now.object_active != before.object_active,
                               in_chain_of( own_bits, now, before, eChange::Active | eChange::Reparented ),
                               "object active changed without Active" );
                        check( now.layer != before.layer, object_bits( now.object ) & static_cast<uint16_t>( eChange::Layer ),
                               "layer changed without Layer" );
                        check( now.has_animator != before.has_animator,
                               object_bits( now.object ) & ( eChange::Added | eChange::Removed ),
                               "animator added or removed without a signal" );
                        check( now.has_animator && before.has_animator &&
                                       ( now.animator_frame != before.animator_frame ||
                                         std::memcmp( now.animator_values, before.animator_values,
                                                      sizeof( now.animator_values ) ) != 0 ),
                               ( ( component_bits( now.animator_id ) | component_bits( before.animator_id ) ) &
                                 ( eChange::Modified | eChange::Added | eChange::Owner ) ) ||
                                       ( own_bits( now.object ) & ( eChange::Added | eChange::Removed ) ),
                               "animator values changed without Modified" );
                        check( now.shape != before.shape || now.animations != before.animations || !SameMeshes( now, before ),
                               ( component_bits( id ) & ( eChange::Modified | eChange::Owner | eChange::Added ) ) ||
                                       RevisionMoved( now, before ),
                               "shape, mesh or material changed without Modified or a resource revision" );

                        changed += any ? 1 : 0;
                    }

                    for ( const auto& [ id, before ] : previous )
                    {
                        if ( !current.contains( id ) && !( component_bits( id ) & ( eChange::Removed | eChange::Owner ) ) &&
                             !( object_bits( before.object ) & static_cast<uint16_t>( eChange::Removed ) ) )
                        {
                            miss( id, before.object, "renderer removed without Removed" );
                        }
                    }
                }

                ++frames;
                if ( frames <= warmup )
                {
                    misses.clear();
                }
                if ( !misses.empty() )
                {
                    for ( const std::string& line : misses )
                    {
                        Report( line );
                    }
                    __fastfail( FAST_FAIL_FATAL_APP_EXIT );
                }
                if ( frames % 60 == 0 )
                {
                    Report( "DIRTY ok frames=" + std::to_string( frames ) + " tracked=" + std::to_string( current.size() ) +
                            " changed=" + std::to_string( changed ) );
                }

                previous       = std::move( current );
                previous_scene = scene_id;
                primed         = true;
            }
        };

        // Scenario: changes the active scene through one path every 10 frames (KOLIBRI_DIRTY_SCENARIO).
        struct Scenario
        {
            std::string                   which;
            bool                          built = false;
            uint64_t                      start = 0;
            Strong<Abstracts::ObjectBase> parents[ 2 ];
            Strong<Abstracts::ObjectBase> child;
            Strong<Abstracts::ObjectBase> extra;
            Weak<Resources::Shape>        shapes[ 2 ];
            Weak<Resources::Texture>      textures[ 2 ];
            size_t                        parent_index = 0;
            int                           play_toggles = 0;

            bool Wants( const char* name ) const
            {
                return which == "all" || ( "," + which + "," ).find( std::string( "," ) + name + "," ) != std::string::npos;
            }

            static Strong<Abstracts::ObjectBase> MakeObject( Scene& scene, const char* name, const Weak<Resources::Shape>& shape )
            {
                const Strong<Abstracts::ObjectBase> object = ObjectFactory::GetGenerator( Object::StaticTypeHash() )();
                object->SetName( name );
                object->AddComponent<Components::Transform>();
                if ( shape.lock() )
                {
                    object->AddComponent<Components::ModelRenderer>().lock()->SetShape( shape );
                }
                scene.AddGameObject( RESERVED_LAYER_DEFAULT, object );
                return object;
            }

            void Build( Scene& scene )
            {
                for ( const Weak<Abstracts::Component>& weak : scene.GetCachedComponentsConcurrent<Components::ModelRenderer>() )
                {
                    const auto renderer = weak.lock();
                    if ( !renderer )
                    {
                        continue;
                    }
                    // An empty Weak does not report expired(): test what lock() returns.
                    const auto shape = static_cast<const Components::ModelRenderer*>( renderer.get() )->GetShape().lock();
                    if ( !shape || shape->GetMeshes().empty() || shape.get() == shapes[ 0 ].lock().get() )
                    {
                        continue;
                    }
                    shapes[ shapes[ 0 ].lock() ? 1 : 0 ] = shape;
                    if ( shapes[ 1 ].lock() )
                    {
                        break;
                    }
                }
                if ( !shapes[ 0 ].lock() )
                {
                    return;
                }
                if ( !shapes[ 1 ].lock() )
                {
                    shapes[ 1 ] = shapes[ 0 ];
                }
                for ( size_t i = 0; i < 2; ++i )
                {
                    if ( const auto material = shapes[ i ].lock()->GetMaterial( 0 ).lock() )
                    {
                        textures[ i ] = material->GetTextures()[ 1 ];
                    }
                }

                parents[ 0 ] = MakeObject( scene, "DirtyParentA", {} );
                parents[ 1 ] = MakeObject( scene, "DirtyParentB", {} );
                child        = MakeObject( scene, "DirtyChild", shapes[ 0 ] );
                parents[ 0 ]->AddChild( child );
                built = true;
            }

            void Step( const uint64_t frame )
            {
                const Strong<Scene> scene = Managers::SceneManager::GetInstance().GetActiveScene().lock();
                if ( !scene )
                {
                    return;
                }
                if ( !built )
                {
                    if ( frame >= 30 )
                    {
                        Build( *scene );
                        start = frame;
                    }
                    return;
                }

                const uint64_t t = frame - start;
                if ( t == 0 || t % 10 != 0 )
                {
                    return;
                }
                // While playing, the rendered scene is a copy; the scenario objects belong to the edited scene. Only
                // the play/stop step runs until Stop.
                const bool playing = Managers::SceneManager::GetInstance().IsPlaying();
                if ( playing && ( t / 10 ) % 10 != 9 )
                {
                    return;
                }

                const auto renderer = child->GetComponent<Components::ModelRenderer>().lock();
                switch ( ( t / 10 ) % 10 )
                {
                case 0:
                    if ( Wants( "move" ) )
                    {
                        // ancestor_transform may have taken the parent's Transform away.
                        if ( const auto transform = parents[ parent_index ]->GetComponent<Components::Transform>().lock() )
                        {
                            transform->SetLocalPosition( transform->GetLocalPosition() + Vector3( 0.01f, 0.f, 0.f ) );
                        }
                    }
                    break;
                case 1:
                    if ( Wants( "reparent" ) )
                    {
                        parents[ parent_index ]->DetachChild( child->GetLocalID() );
                        parent_index = 1 - parent_index;
                        parents[ parent_index ]->AddChild( child );
                    }
                    break;
                case 2:
                    if ( Wants( "component_active" ) && renderer )
                    {
                        renderer->SetActive( !renderer->GetActive() );
                    }
                    break;
                case 3:
                    if ( Wants( "object_active" ) )
                    {
                        parents[ parent_index ]->SetActive( !parents[ parent_index ]->GetActive() );
                    }
                    break;
                case 4:
                    if ( Wants( "layer" ) )
                    {
                        scene->ChangeLayer( child->GetLayer() == RESERVED_LAYER_DEFAULT ? RESERVED_LAYER_ENVIRONMENT
                                                                                       : RESERVED_LAYER_DEFAULT,
                                            child->GetID() );
                    }
                    break;
                case 5:
                    if ( Wants( "shape" ) && renderer )
                    {
                        const bool first = renderer->GetShape().lock().get() == shapes[ 0 ].lock().get();
                        renderer->SetShape( shapes[ first ? 1 : 0 ] );
                    }
                    break;
                case 6:
                    if ( Wants( "material" ) && renderer && textures[ 0 ].lock() && textures[ 1 ].lock() )
                    {
                        if ( const auto shape = renderer->GetShape().lock() )
                        {
                            if ( const auto material = shape->GetMaterial( 0 ).lock() )
                            {
                                const bool first = material->GetTextures()[ 1 ].lock().get() == textures[ 0 ].lock().get();
                                material->SetTexture( textures[ first ? 1 : 0 ], 1, false );
                            }
                        }
                    }
                    break;
                case 7:
                    if ( Wants( "add_remove" ) )
                    {
                        if ( extra )
                        {
                            scene->RemoveGameObject( extra->GetID(), extra->GetLayer() );
                            extra.reset();
                        }
                        else
                        {
                            extra = MakeObject( *scene, "DirtyExtra", shapes[ 0 ] );
                        }
                    }
                    break;
                case 8:
                    if ( Wants( "ancestor_transform" ) )
                    {
                        // The child's world matrix skips a parent without a Transform.
                        const Strong<Abstracts::ObjectBase>& parent = parents[ parent_index ];
                        if ( parent->GetComponent<Components::Transform>().lock() )
                        {
                            parent->RemoveComponent<Components::Transform>();
                        }
                        else
                        {
                            parent->AddComponent<Components::Transform>().lock()->SetLocalPosition( Vector3( 0.f, 0.5f, 0.f ) );
                        }
                    }
                    break;
                case 9:
                    // One play/stop cycle: a second Play after Stop crashes in ShadowManager::ClearShadowMaps (engine bug,
                    // not part of this check).
                    if ( Wants( "play_stop" ) && play_toggles < 2 )
                    {
                        ++play_toggles;
                        Managers::SceneManager& manager = Managers::SceneManager::GetInstance();
                        if ( manager.IsPlaying() )
                        {
                            manager.Stop();
                        }
                        else
                        {
                            manager.Play();
                        }
                    }
                    break;
                }
            }
        };
    }

    void Run( const Scene& scene )
    {
        // Cost measurement: KOLIBRI_DIRTY_STRESS=N moves N rendered objects every frame (one setter each).
        static const int stress = std::atoi( GetDiagSwitch( "KOLIBRI_DIRTY_STRESS" ).value_or( "0" ).c_str() );
        if ( stress > 0 )
        {
            static float step  = 0.001f;
            int          moved = 0;
            step               = -step;
            for ( const Weak<Abstracts::Component>& weak : scene.GetCachedComponentsConcurrent<Components::ModelRenderer>() )
            {
                if ( moved >= stress )
                {
                    break;
                }
                if ( const auto component = weak.lock() )
                {
                    if ( const auto object = component->GetOwner().lock() )
                    {
                        if ( const auto transform = object->GetComponent<Components::Transform>().lock() )
                        {
                            transform->SetLocalPosition( transform->GetLocalPosition() + Vector3( step, 0.f, 0.f ) );
                            ++moved;
                        }
                    }
                }
            }
        }

        static const std::string mode = GetDiagSwitch( "KOLIBRI_DIRTY_CHECK" ).value_or( "" );
        if ( mode == "collect" )
        {
            // Cost measurement: a subscribed collector drained every frame, nothing compared.
            static ChangeCollector          collector;
            static std::vector<ChangeEvent> events;
            static uint64_t                 frames = 0, entities = 0;
            static std::ofstream            output{ "dirtycheck.txt", std::ios::app };
            collector.Drain( events );
            entities += events.size();
            if ( ++frames % 600 == 0 )
            {
                output << "DIRTY collect frames=" << frames << " entities/frame=" << ( entities / 600.0 ) << "\n";
                output.flush();
                entities = 0;
            }
            return;
        }
        if ( mode != "1" )
        {
            return;
        }

        static Checker  checker;
        static Scenario scenario{ GetDiagSwitch( "KOLIBRI_DIRTY_SCENARIO" ).value_or( "" ) };

        checker.Check( scene );
        if ( !scenario.which.empty() )
        {
            scenario.Step( checker.frames );
        }
    }
}
#else
namespace Engine::DirtyCheck
{
    void Run( const Scene& ) {}
}
#endif
