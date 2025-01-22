#include "../Public/RenderPipeline.h"
#include "../Public/Renderer.h"

#include "Source/Runtime/Core/ModuleManager/Public/ModuleManager.h"
#include "CoreModuel/Public/CoreModule.h"

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

	void RenderPipeline::BindConstantBuffers(const GraphicInterfaceContextPrimitive* context)
	{
		m_wvp_buffer_cb_.Bind(context);
		m_param_buffer_cb_.Bind(context);
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
		}

		if (!m_param_buffer_cb_)
		{
			m_param_buffer_cb_ = gi.GetConstantBuffer<CBs::ParamCB>();
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

MODULE_IMPL(Engine::RenderPipelineModule, RenderPipeline)

void Engine::RenderPipelineModule::Initialize()
{
	CoreModule::GetContext().AddManager(
		CoreLoop::LOOP_TYPE_RENDER,
		&Managers::RenderPipeline::GetInstance,
		&Managers::Renderer::GetInstance);
}
void Engine::RenderPipelineModule::Shutdown()
{
	CoreModule::GetContext().RemoveManager(
		CoreLoop::LOOP_TYPE_RENDER,
		&Managers::RenderPipeline::GetInstance,
		&Managers::Renderer::GetInstance);
}

bool Engine::RenderPipelineModule::DynamicLoadable()
{
	return true;
}
