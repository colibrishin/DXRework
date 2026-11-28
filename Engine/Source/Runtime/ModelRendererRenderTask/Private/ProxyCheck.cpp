#include "ProxyCheck.h"

#if WITH_EDITOR || WITH_DEBUG
#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>
#include <windows.h>

#include "ChangeSignal.h"
#include "InstanceBuild.h"
#include "ModelRenderer.h"
#include "ObjectBase.h"
#include "Scene.h"

namespace Engine::ProxyCheck
{
    namespace
    {
        constexpr size_t texture_slot_begin = 10; // InstanceSB::SetTextureSlot: i_param[10..17] slots, [18..25] enabled
        constexpr size_t texture_slot_end   = 10 + 2 * g_max_texture_per_material;

        struct Record
        {
            size_t                    domain = 0;
            const void*               shader = nullptr;
            const void*               mesh   = nullptr;
            GlobalEntityID            object = 0;
            std::array<const void*, 10> textures{};
            std::string               values; // instance bytes, texture slot fields cleared

            auto Key() const
            {
                return std::tie( domain, shader, mesh, object, textures, values );
            }
        };

        std::vector<Record> Collect( RenderMap* render_map, const size_t map_size, std::vector<bool>& has_key )
        {
            std::vector<Record> out;
            has_key.assign( map_size, false );
            for ( size_t domain = 0; domain < map_size; ++domain )
            {
                RenderMap::const_accessor acc;
                if ( !render_map[ domain ].find( acc, Components::ModelRenderer::StaticTypeHash() ) )
                {
                    continue;
                }
                has_key[ domain ] = true;
                for ( auto& [ shader, meshes ] : acc->second )
                {
                    for ( auto& [ mesh, instances ] : meshes )
                    {
                        for ( const InstancePair& pair : instances )
                        {
                            Record& record = out.emplace_back();
                            record.domain  = domain;
                            record.shader  = shader.get();
                            record.mesh    = mesh.get();
                            record.object  = pair.object ? pair.object->GetID() : 0;
                            for ( size_t i = 0; i < pair.textures.size(); ++i )
                            {
                                record.textures[ i ] = pair.textures[ i ].get();
                            }
                            for ( size_t i = 0; i < pair.reservedTextures.size(); ++i )
                            {
                                record.textures[ pair.textures.size() + i ] = pair.reservedTextures[ i ].get();
                            }
                            Graphics::SBs::InstanceSB copy = *pair.instance;
                            for ( size_t i = texture_slot_begin; i < texture_slot_end; ++i )
                            {
                                copy.SetParam<int>( static_cast<UINT>( i ), 0 );
                            }
                            record.values.assign( reinterpret_cast<const char*>( &copy ), sizeof( copy ) );
                        }
                    }
                }
            }
            std::sort( out.begin(), out.end(), []( const Record& a, const Record& b ) { return a.Key() < b.Key(); } );
            return out;
        }

        // Buffers for the reference build, separate from the instance task's pool (reset by the build before use).
        struct ScratchPool
        {
            std::mutex                                               mutex;
            std::vector<std::unique_ptr<Graphics::SBs::InstanceSB>> buffers;
            size_t                                                   used = 0;

            void Acquire( const size_t count, aligned_vector<Graphics::SBs::InstanceSB*>& out )
            {
                std::lock_guard lock( mutex );
                for ( size_t i = 0; i < count; ++i )
                {
                    if ( used == buffers.size() )
                    {
                        buffers.push_back( std::make_unique<Graphics::SBs::InstanceSB>() );
                    }
                    out.push_back( buffers[ used++ ].get() );
                }
            }
        };

        void Fail( std::ofstream& output, const std::string& what )
        {
            output << "PROXY MISMATCH " << what << "\n";
            output.flush();
            OutputDebugStringA( ( "PROXY MISMATCH " + what + "\n" ).c_str() );
            __fastfail( FAST_FAIL_FATAL_APP_EXIT );
        }

        std::string Describe( const Record& r )
        {
            return "domain=" + std::to_string( r.domain ) + " object=" + std::to_string( r.object ) +
                   " shader=" + std::to_string( reinterpret_cast<uintptr_t>( r.shader ) ) +
                   " mesh=" + std::to_string( reinterpret_cast<uintptr_t>( r.mesh ) );
        }
    }

    void Run( const Scene& scene, RenderMap* render_map, const size_t map_size )
    {
        static const bool enabled = GetDiagSwitch( "KOLIBRI_PROXY_CHECK" ).value_or( "" ) == "1";
        if ( !enabled )
        {
            return;
        }
        static const bool  mutate_world = GetDiagSwitch( "KOLIBRI_PROXY_MUTATE" ).value_or( "" ) == "world";
        static std::ofstream output{ "proxycheck.txt", std::ios::app };
        static ScratchPool  pool;
        static std::unique_ptr<RenderMap[]> reference;
        static uint64_t     frames = 0, compared = 0;
        if ( !reference )
        {
            reference = std::make_unique<RenderMap[]>( map_size );
        }

        if ( mutate_world && frames == 60 )
        {
            RenderMap::const_accessor acc;
            for ( size_t domain = 0; domain < map_size; ++domain )
            {
                if ( render_map[ domain ].find( acc, Components::ModelRenderer::StaticTypeHash() ) && !acc->second.empty() )
                {
                    auto& meshes = acc->second.begin()->second;
                    if ( !meshes.empty() && !meshes.begin()->second.empty() )
                    {
                        Graphics::SBs::InstanceSB& sb = *meshes.begin()->second.front().instance;
                        Matrix world                  = sb.GetParam<Matrix>( 0 );
                        world._41 += 1.f;
                        sb.SetParam( 0, world );
                        break;
                    }
                }
            }
        }

        pool.used = 0;
        BuildOptions options;
        options.acquire = [ & ]( const size_t count, aligned_vector<Graphics::SBs::InstanceSB*>& out )
        {
            pool.Acquire( count, out );
        };
        options.reset_instances = true;
        options.skip_hidden    = true;
        BuildAllInstances( scene, reference.get(), map_size, options );

        std::vector<bool> subject_keys, reference_keys;
        const std::vector<Record> subject   = Collect( render_map, map_size, subject_keys );
        const std::vector<Record> expected  = Collect( reference.get(), map_size, reference_keys );

        for ( size_t domain = 0; domain < map_size; ++domain )
        {
            if ( subject_keys[ domain ] != reference_keys[ domain ] )
            {
                Fail( output, "frame=" + std::to_string( frames ) + " key presence differs in domain " + std::to_string( domain ) );
            }
        }
        if ( subject.size() != expected.size() )
        {
            Fail( output, "frame=" + std::to_string( frames ) + " instance count " + std::to_string( subject.size() ) +
                                  " vs reference " + std::to_string( expected.size() ) );
        }
        for ( size_t i = 0; i < subject.size(); ++i )
        {
            if ( subject[ i ].Key() != expected[ i ].Key() )
            {
                const bool same_slot = subject[ i ].domain == expected[ i ].domain && subject[ i ].object == expected[ i ].object;
                std::string detail;
                if ( same_slot )
                {
                    detail = subject[ i ].textures != expected[ i ].textures ? " textures differ" : " values differ at bytes";
                    const std::string& a = subject[ i ].values;
                    const std::string& b = expected[ i ].values;
                    for ( size_t k = 0, shown = 0; k < a.size() && shown < 12; k += 4 )
                    {
                        if ( std::memcmp( a.data() + k, b.data() + k, 4 ) != 0 )
                        {
                            int x, y;
                            std::memcpy( &x, a.data() + k, 4 );
                            std::memcpy( &y, b.data() + k, 4 );
                            detail += " " + std::to_string( k ) + ":" + std::to_string( x ) + "/" + std::to_string( y );
                            ++shown;
                        }
                    }
                }
                Fail( output, "frame=" + std::to_string( frames ) + " " + Describe( subject[ i ] ) +
                                      ( same_slot ? detail : " vs reference " + Describe( expected[ i ] ) ) );
            }
        }

        for ( size_t domain = 0; domain < map_size; ++domain )
        {
            reference[ domain ].erase( Components::ModelRenderer::StaticTypeHash() );
        }
        compared += subject.size();
        if ( ++frames % 600 == 0 )
        {
            output << "PROXY ok frames=" << frames << " instances/frame=" << ( compared / 600.0 ) << "\n";
            output.flush();
            compared = 0;
        }
    }
}
#else
namespace Engine::ProxyCheck
{
    void Run( const Scene&, RenderMap*, size_t ) {}
}
#endif
