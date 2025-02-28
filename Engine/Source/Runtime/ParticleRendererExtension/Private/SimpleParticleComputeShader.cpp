#include "SimpleParticleComputeShader.h"
#include "SimpleParticleComputeShader.generated.h"

#include "ParticleRenderer.h"
#include "Texture2D.h"

#if WITH_EDITOR
void Engine::Resources::SimpleParticleComputeShader::OnUIUpdate(UIContext* const parent, const float dt)
{
    if (parent)
    {
        ComputeShader::OnUIUpdate(parent, dt);
    }
}

void Engine::Resources::SimpleParticleComputeShader::OnUIUpdateParam(UIContext *const   parent,
                                                                     const float        dt,
                                                                     ParamBase &        local_param,
                                                                     InstanceParticles &instances)
{
    if ( parent )
    {
        IUIAPI &ui = g_ui_accessor.GetInterface();
        *parent |= ui.NewCheckbox(this, "Scaling",
                                  { "Scaling", local_param.GetParam<bool>(param_scaling_active_slot), true });
        if ( local_param.GetParam<bool>(param_scaling_active_slot) )
        {
            *parent |= ui.NewLabelAndFloat(this, "ScaleMin",
                                           { "Scale Min",
                                             local_param.GetParam<float>(param_scaling_min_slot),
                                             0,
                                             0,
                                             std::numeric_limits<float>::max(),
                                             true });
            *parent |= ui.NewLabelAndFloat(this, "ScaleMax",
                                           { "Scale Max",
                                             local_param.GetParam<float>(param_scaling_max_slot),
                                             0,
                                             0,
                                             std::numeric_limits<float>::max(),
                                             true });
        }

        static Vector3 linear_min, linear_max;
        *parent |= ui.NewLabelAndVec3(this, "LinearMin",
                                      { "Linear Spread Min",
                                        &linear_min.x,
                                        0,
                                        0,
                                        std::numeric_limits<float>::max(),
                                        true });
        *parent |= ui.NewLabelAndVec3(this, "LinearMin",
                                      { "Linear Spread Max",
                                        &linear_max.x,
                                        0,
                                        0,
                                        std::numeric_limits<float>::max(),
                                        true });
        (*parent |= ui.NewButton(this, "LinearButton", { "Linear Spread" })).SetFunction(
            [ this, &instances, &local_param ]() { LinearSpread(linear_min, linear_max, instances, local_param); });
    }
}
#endif

void Engine::Resources::SimpleParticleComputeShader::preDispatch(
    const IGraphicContext* context, SBs::LocalParamSB& param, const float dt)
{
    IGraphicAPI& gi = g_graphic_accessor.GetInterface();
    gi.TransitTo( context, m_noises_[ 0 ].get(), BIND_TYPE_SRV );
    gi.TransitTo( context, m_noises_[ 1 ].get(), BIND_TYPE_SRV );
    gi.TransitTo( context, m_noises_[ 2 ].get(), BIND_TYPE_SRV );

    gi.Bind( context, m_noises_[ 0 ].get(), BIND_TYPE_SRV, BIND_SLOT_TEX, 0 );
    gi.Bind( context, m_noises_[ 1 ].get(), BIND_TYPE_SRV, BIND_SLOT_TEX, 1 );
    gi.Bind( context, m_noises_[ 2 ].get(), BIND_TYPE_SRV, BIND_SLOT_TEX, 2 );

    param.SetParam<float>( param_dt_slot, dt );

    auto rng           = getRandomEngine();
    const auto rng_val = rng() % random_texture_size;
    param.SetParam( param_random_value_slot, static_cast<int>( rng_val ) );
}

void Engine::Resources::SimpleParticleComputeShader::postDispatch(
    const IGraphicContext* context, SBs::LocalParamSB& param, const float dt)
{
    IGraphicAPI& gi = g_graphic_accessor.GetInterface();
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
    config.SetParam( scaling, param_scaling_active_slot );
}

void Engine::Resources::SimpleParticleComputeShader::SetScalingParam(float min, float max, ParamBase& config)
{
    config.SetParam( min, param_scaling_min_slot );
    config.SetParam( max, param_scaling_max_slot );
}

void Engine::Resources::SimpleParticleComputeShader::LinearSpread(
    const Vector3& local_min, const Vector3& local_max, InstanceParticles& particles, const ParamBase& config)
{
    const auto count = particles.size();
    for ( auto i = 0; i < count; ++i )
    {
        auto&       instance       = particles[i];
        auto        world          = instance.GetWorld().Transpose();

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
