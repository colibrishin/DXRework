#include "RenderPipeline.h"
#include "RenderPipeline.generated.h"

#include "CoreModule.h"

#include "Camera.h"
#include "Light.h"
#include "Transform.h"
#include "Scene.h"
#include "Renderer.h"

namespace Engine::Managers
{
	using namespace Resources;

	void RenderPipeline::SetPerspectiveMatrix(const CBs::PerspectiveCB& matrix)
	{
		m_wvp_buffer_ = matrix;
		ConstantBufferGuard();
		m_wvp_buffer_cb_.SetData(&m_wvp_buffer_);
	}

    void RenderPipeline::UpdateLights(const IGraphicContext* context, const SBs::LightSB* lights, const size_t count)
	{
	    StructuredBufferGuard();
        m_light_buffer_sb_.SetData(context, count, lights);
	}

    void RenderPipeline::BindConstantBuffers(const IGraphicContext* context) const
	{
		m_wvp_buffer_cb_.Bind(context);
		m_param_buffer_cb_.Bind(context);
	}

    void RenderPipeline::SetRaytracing( const bool flag )
	{
	    TaskScheduler::GetInstance().AddTask( TASK_TOGGLE_RASTER, {flag, 0.f} );
	}

    const ConstantBufferTypeProxy<CBs::PerspectiveCB>& RenderPipeline::GetPerspectiveCB() const
	{
	    return m_wvp_buffer_cb_;
	}

    const ConstantBufferTypeProxy<CBs::ParamCB>& RenderPipeline::GetParamCB() const
	{
	    return m_param_buffer_cb_;
	}

    const StructuredBufferTypeProxy<SBs::LightSB>& RenderPipeline::GetLightSB() const
	{
	    return m_light_buffer_sb_;
	}

    const Viewport& RenderPipeline::GetViewport() const
	{
		return m_viewport_;
	}
	
	RenderPipeline::~RenderPipeline()
	{
	    Renderer::GetInstance().UnregisterStructuredBuffer(&m_light_buffer_sb_);
	}

	void RenderPipeline::ConstantBufferGuard()
	{
		IGraphicAPI& gi = s_ga.GetInterface();

		if (!m_wvp_buffer_cb_)
		{
			m_wvp_buffer_cb_ = gi.GetConstantBuffer<CBs::PerspectiveCB>();
			m_wvp_buffer_cb_.SetData(nullptr);
		}

		if (!m_param_buffer_cb_)
		{
			m_param_buffer_cb_ = gi.GetConstantBuffer<CBs::ParamCB>();
			m_param_buffer_cb_.SetData(nullptr);
		}
	}

    void RenderPipeline::StructuredBufferGuard()
	{
	    if (!m_light_buffer_sb_)
	    {
	        IGraphicAPI& gi = s_ga.GetInterface();
	        m_light_buffer_sb_ = gi.GetStructuredBuffer<SBs::LightSB>();
	    }
	}

    void RenderPipeline::InitializeViewport()
	{
		m_viewport_ = {
				0,
				0,
				CFG_WIDTH,
				CFG_HEIGHT,
				0.f,
				1.f
			};
	}

	void RenderPipeline::Initialize()
	{
		InitializeViewport();
	    
	    IGraphicAPI& gi = s_ga.GetInterface();
	    m_light_buffer_sb_ = gi.GetStructuredBuffer<SBs::LightSB>();

	    Renderer::GetInstance().RegisterStructuredBuffer(&m_light_buffer_sb_);
	}

	void RenderPipeline::PreUpdate(const float dt) {}

	void RenderPipeline::PreRender(const float dt)
	{
		IGraphicAPI& gi = s_ga.GetInterface();
		gi.ClearRenderTarget();

		if (const Strong<Scene>& scene = SceneManager::GetInstance().GetActiveScene().lock())
		{
			if (const Strong<Objects::Camera>& camera = scene->GetMainCamera().lock())
			{
				SetPerspectiveMatrix(camera->GetPerspectiveCB());
			}

            const auto &lights = scene->GetLights();

            // Notify the number of lights to the shader.
            constexpr size_t light_slot = 0;
            SetParam<int>( static_cast<UINT>( lights.size() ), light_slot );

            // Build light information structured buffer.
            std::vector<SBs::LightSB> light_buffer;
            light_buffer.reserve( lights.size() );

            for ( const auto &light : lights )
            {
                if ( const Strong<Objects::Light> &locked = light.lock() )
                {
                    const auto tr    = locked->GetComponent<Components::Transform>().lock();
                    const auto world = tr->GetWorldMatrix();

                    light_buffer.emplace_back
                            ( world.Transpose(), locked->GetColor(), locked->GetType(), locked->GetRange() );
                }
            }

            const auto &context   = gi.GetNewContext( 0, false, L"Light Structured Buffer Update" );
            const auto &primitive = context.GetPointers();
		    primitive.commandList->SoftReset();
            CheckSize<UINT>( light_buffer.size(), L"Warning: Light buffer size is too big!" );
            UpdateLights( &primitive, light_buffer.data(), light_buffer.size() );
		    primitive.commandList->FlagReady();
		}
	}

	void RenderPipeline::Update(const float dt) {}

	void RenderPipeline::Render(const float dt) {}

	void RenderPipeline::FixedUpdate(const float dt) {}

	void RenderPipeline::PostRender(const float dt)
	{
		s_ga.GetInterface().Present();
		s_ga.GetInterface().WaitForNextFrame();
	}

	void RenderPipeline::PostUpdate(const float dt) {}

#if WITH_EDITOR
    void RenderPipeline::OnUIUpdate( UIContext * const parent, const float dt )
    {
	    IUIAPI& ui = s_uia.GetInterface();
        if ( UIContext context = ui.NewContext
                ( ui.NewDialog( this, "RenderPipelineDialog", { "RenderPipeline", m_ui_info_.dialogOpened } ) ) )
        {
            (context |= ui.NewCheckbox( this, "RaytracingCheckbox", {"Raytracing", m_b_raytracing_, CFG_RAYTRACING } )).SetFunction( [this]()
            {
                SetRaytracing( m_b_raytracing_ );
            } );
        }
    }
#endif

} // namespace Engine::Manager::Graphics
