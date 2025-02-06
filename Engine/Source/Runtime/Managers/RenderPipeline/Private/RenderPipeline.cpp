#include "../Public/RenderPipeline.h"

#include "RaytracingRenderer.h"
#include "RenderPipeline.generated.h"
#include "../Public/Renderer.h"


#include "CoreModule/Public/CoreModule.h"

#include "Objects/Camera/Public/Camera.h"

#include "Scene/Public/Scene.h"

namespace Engine::Managers
{
	using namespace Resources;

	void RenderPipeline::SetPerspectiveMatrix(const CBs::PerspectiveCB& matrix)
	{
		m_wvp_buffer_ = matrix;
		ConstantBufferGuard();
		m_wvp_buffer_cb_.SetData(&m_wvp_buffer_);
	}

    void RenderPipeline::UpdateLights(const GraphicInterfaceContextPrimitive* context, const SBs::LightSB* lights, const size_t count)
	{
	    StructuredBufferGuard();
        m_light_buffer_sb_.SetData(context, count, lights);
	}

    void RenderPipeline::BindConstantBuffers(const GraphicInterfaceContextPrimitive* context) const
	{
		m_wvp_buffer_cb_.Bind(context);
		m_param_buffer_cb_.Bind(context);
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
	
	RenderPipeline::~RenderPipeline() {}

	void RenderPipeline::ConstantBufferGuard()
	{
		GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();

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
	        GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();
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
	}

	void RenderPipeline::PreUpdate(const float dt) {}

	void RenderPipeline::PreRender(const float dt)
	{
		GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();
		gi.ClearRenderTarget();

		if (const Strong<Scene>& scene = SceneManager::GetInstance().GetActiveScene().lock())
		{
			if (const Strong<Objects::Camera>& camera = scene->GetMainCamera().lock())
			{
				SetPerspectiveMatrix(camera->GetPerspectiveCB());
			}
		}
	}

	void RenderPipeline::Update(const float dt) {}

	void RenderPipeline::Render(const float dt) {}

	void RenderPipeline::FixedUpdate(const float dt) {}

	void RenderPipeline::PostRender(const float dt)
	{
		GraphicInterfaceAccessor::GetInterface().Present();
		GraphicInterfaceAccessor::GetInterface().WaitForNextFrame();
	}

	void RenderPipeline::PostUpdate(const float dt) {}

} // namespace Engine::Manager::Graphics
