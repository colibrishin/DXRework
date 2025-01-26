#include "SimpleParticleComputeShader.h"
#include "SimpleParticleComputeShader.generated.h"

#include "ParticleRenderer.h"
#include "Texture2D.h"

void Engine::Resources::SimpleParticleComputeShader::preDispatch(
    const GraphicInterfaceContextPrimitive* context, SBs::LocalParamSB& param)
{
    GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();
    gi.TransitTo( context, m_noises_[ 0 ].get(), BIND_TYPE_SRV );
    gi.TransitTo( context, m_noises_[ 1 ].get(), BIND_TYPE_SRV );
    gi.TransitTo( context, m_noises_[ 2 ].get(), BIND_TYPE_SRV );

    gi.Bind( context, m_noises_[ 0 ].get(), BIND_TYPE_SRV, BIND_SLOT_TEX, 0 );
    gi.Bind( context, m_noises_[ 1 ].get(), BIND_TYPE_SRV, BIND_SLOT_TEX, 1 );
    gi.Bind( context, m_noises_[ 2 ].get(), BIND_TYPE_SRV, BIND_SLOT_TEX, 2 );

    auto rng           = getRandomEngine();
    const auto rng_val = rng() % random_texture_size;
    param.SetParam( random_value_slot, static_cast<int>( rng_val ) );
}

void Engine::Resources::SimpleParticleComputeShader::postDispatch(
    const GraphicInterfaceContextPrimitive* context, SBs::LocalParamSB& param)
{
    GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();
    gi.TransitBack( context, m_noises_[ 0 ].get(), BIND_TYPE_SRV );
    gi.TransitBack( context, m_noises_[ 1 ].get(), BIND_TYPE_SRV );
    gi.TransitBack( context, m_noises_[ 2 ].get(), BIND_TYPE_SRV );

    auto rng = getRandomEngine();
    std::ranges::shuffle( m_noises_, rng );
}

void Engine::Resources::SimpleParticleComputeShader::loadDerived()
{
    static std::vector<std::string> f_name = { "./noise0.png", "./noise1.png", "./noise2.png" };
    auto rng                               = getRandomEngine();

    std::ranges::shuffle( f_name, rng );

    m_noises_[ 0 ] = Texture2D::Create( "RandomNoiseTexture1", f_name[ 0 ], GenericTextureDescription{ } );
    m_noises_[ 1 ] = Texture2D::Create( "RandomNoiseTexture2", f_name[ 1 ], GenericTextureDescription{ } );
    m_noises_[ 2 ] = Texture2D::Create( "RandomNoiseTexture3", f_name[ 2 ], GenericTextureDescription{ } );
}

void Engine::Resources::SimpleParticleComputeShader::unloadDerived()
{
    m_noises_[ 0 ].reset();
    m_noises_[ 1 ].reset();
    m_noises_[ 2 ].reset();
}

void Engine::Resources::SimpleParticleComputeShader::SetScaling(bool scaling, ParamBase& config)
{
    config.SetParam( scaling, scaling_active_slot );
}

void Engine::Resources::SimpleParticleComputeShader::SetScalingParam(float min, float max, ParamBase& config)
{
    config.SetParam( min, scaling_min_slot );
    config.SetParam( max, scaling_max_slot );
}

void Engine::Resources::SimpleParticleComputeShader::LinearSpread(
    const Vector3& local_min, const Vector3& local_max, InstanceParticles& particles, const ParamBase& config)
{
    const auto count = particles.size();
    for ( auto i = 0; i < count; ++i )
    {
        auto& instance = particles[ i ];
        auto world     = instance.GetWorld().Transpose();

        const auto new_pos = Vector3::Lerp( local_min,
                                            local_max,
                                            static_cast<float>( i ) / static_cast<float>( count ) );

        world *= Matrix::CreateScale( config.GetParam<float>( Components::ParticleRenderer::size_slot ) ) *
            Matrix::CreateTranslation( new_pos - world.Translation() );
        instance.SetWorld( world.Transpose() );
    }
}

std::mt19937_64 Engine::Resources::SimpleParticleComputeShader::getRandomEngine()
{
    std::random_device dev;
    std::default_random_engine init_seed( dev() );

    std::uniform_int_distribution initial_dist( std::numeric_limits<unsigned long long>().min(),
                                                std::numeric_limits<unsigned long long>().max() );

    std::seed_seq rand_seq{
        initial_dist( init_seed ), initial_dist( init_seed ), initial_dist( init_seed ), initial_dist( init_seed ),
        initial_dist( init_seed ), initial_dist( init_seed ), initial_dist( init_seed ), initial_dist( init_seed ),
        initial_dist( init_seed ), initial_dist( init_seed ), initial_dist( init_seed ), initial_dist( init_seed ),
        initial_dist( init_seed ), };

    const std::mt19937_64 rng( rand_seq );
    return rng;
}
