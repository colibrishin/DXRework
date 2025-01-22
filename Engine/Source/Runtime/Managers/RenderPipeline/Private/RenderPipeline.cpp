#include "../Public/RenderPipeline.h"
#include "../Public/Renderer.h"

namespace Engine::Managers
{
	using namespace Resources;

	void RenderPipeline::SetPerspectiveMatrix(const CBs::PerspectiveCB& matrix)
	{
		m_wvp_buffer_ = matrix;
		m_perspective_cb_task_->SetData(&m_wvp_buffer_, 1);
	}

	ViewportRenderPrerequisiteTask* RenderPipeline::GetDefaultViewportPrerequisiteTask() const
	{
		m_viewport_task_->SetViewport(m_viewport_);
		return m_viewport_task_.get();
	}

	ShaderRenderPrerequisiteTask* RenderPipeline::GetShaderRenderPrerequisiteTask(
		const GraphicPrimitiveShader* shader) const
	{
		if (shader)
		{
			m_graphics_shader_task_->SetShader(shader);
			m_graphics_shader_task_->SetPipelineSignature(m_graphics_primitive_pipeline_->GetNativePipeline());
			return m_graphics_shader_task_.get();
		}

		return nullptr;
	}

	PipelineRenderPrerequisiteTask* RenderPipeline::GetPipelineRenderPrerequisiteTask() const
	{
		return m_pipeline_task_.get();
	}

	ConstantBufferRenderPrerequisiteTask<CBs::PerspectiveCB>* RenderPipeline::GetPerspectiveConstantBufferRenderPrerequisiteTask() const
	{
		if (!m_perspective_cb_task_)
		{
			throw std::runtime_error("Perspective constant buffer task has not assigned to");
		}

		return m_perspective_cb_task_.get();
	}

	ConstantBufferRenderPrerequisiteTask<CBs::ParamCB>* RenderPipeline::GetParamConstantBufferRenderPrerequisiteTask() const
	{
		if (!m_param_cb_task_)
		{
			throw std::runtime_error("Param constant buffer task has not assigned to");
		}

		return m_param_cb_task_.get();
	}

	void RenderPipeline::SetPrimitivePipeline(PrimitivePipeline* pipeline)
	{
		m_graphics_primitive_pipeline_ = std::unique_ptr<PrimitivePipeline>(pipeline);
	}

	PrimitivePipeline* RenderPipeline::GetPrimitivePipeline() const
	{
		return m_graphics_primitive_pipeline_.get();
	}

	RenderPipeline::~RenderPipeline() { }

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
		PrecompileShaders();
		InitializeViewport();
		m_graphics_primitive_pipeline_->Generate();
		m_graphics_shader_task_->SetPipelineSignature(m_graphics_primitive_pipeline_->GetNativePipeline());

		Managers::Renderer::GetInstance().RegisterRenderPassPrerequisite(GetDefaultViewportPrerequisiteTask());
		Managers::Renderer::GetInstance().RegisterRenderPassPrerequisite(GetPipelineRenderPrerequisiteTask());
		Managers::Renderer::GetInstance().RegisterRenderPassPrerequisite(GetPerspectiveConstantBufferRenderPrerequisiteTask());
		Managers::Renderer::GetInstance().RegisterRenderPassPrerequisite(GetParamConstantBufferRenderPrerequisiteTask());
	}

	void RenderPipeline::PrecompileShaders()
	{
		/*
		Shader::Create
				(
				 "default", "./default.hlsl", SHADER_DOMAIN_OPAQUE,
				 SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
				 SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
				 D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				 SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS,
				 &g_default_rtv_format, 1, DXGI_FORMAT_D24_UNORM_S8_UINT,
				 D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
				 SAMPLER_SHADOW
				);

		Shader::Create
				(
				 "color", "./color.hlsl", SHADER_DOMAIN_OPAQUE,
				 SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
				 SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
				 D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				 SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS,
				 &g_default_rtv_format, 1, DXGI_FORMAT_D24_UNORM_S8_UINT,
				 D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
				 SAMPLER_SHADOW
				);

		Shader::Create
				(
				 "skybox", "./skybox.hlsl", SHADER_DOMAIN_OPAQUE,
				 SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
				 SHADER_RASTERIZER_CULL_NONE | SHADER_RASTERIZER_FILL_SOLID,
				 D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				 SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS,
				 &g_default_rtv_format, 1, DXGI_FORMAT_D24_UNORM_S8_UINT,
				 D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
				 SAMPLER_SHADOW
				);

		Shader::Create
				(
				 "specular_normal", "./specular_normal.hlsl",
				 SHADER_DOMAIN_OPAQUE,
				 SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
				 SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
				 D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				 SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS,
				 &g_default_rtv_format, 1, DXGI_FORMAT_D24_UNORM_S8_UINT,
				 D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
				 SAMPLER_SHADOW
				);

		Shader::Create
				(
				 "normal", "./normal.hlsl", SHADER_DOMAIN_OPAQUE,
				 SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
				 SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
				 D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				 SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS,
				 &g_default_rtv_format, 1, DXGI_FORMAT_D24_UNORM_S8_UINT,
				 D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
				 SAMPLER_SHADOW
				);

		Shader::Create
				(
				 "refraction", "./refraction.hlsl", SHADER_DOMAIN_POST_PROCESS,
				 SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
				 SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
				 D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				 SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS,
				 &g_default_rtv_format, 1, DXGI_FORMAT_D24_UNORM_S8_UINT,
				 D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
				 SAMPLER_SHADOW
				);

		Shader::Create
				(
				 "specular_tex", "./specular_tex.hlsl", SHADER_DOMAIN_OPAQUE,
				 SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
				 SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
				 D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				 SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS,
				 &g_default_rtv_format, 1, DXGI_FORMAT_D24_UNORM_S8_UINT,
				 D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
				 SAMPLER_SHADOW
				);

		Shader::Create
				(
				 "specular", "./specular.hlsl", SHADER_DOMAIN_OPAQUE,
				 SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
				 SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
				 D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				 SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS,
				 &g_default_rtv_format, 1, DXGI_FORMAT_D24_UNORM_S8_UINT,
				 D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
				 SAMPLER_SHADOW
				);

		const auto billboard = Shader::Create
				(
				 "billboard", "./billboard.hlsl", SHADER_DOMAIN_OPAQUE,
				 SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
				 SHADER_RASTERIZER_CULL_NONE | SHADER_RASTERIZER_FILL_SOLID,
				 D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				 SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS,
				 &g_default_rtv_format, 1, DXGI_FORMAT_D24_UNORM_S8_UINT,
				 D3D_PRIMITIVE_TOPOLOGY_POINTLIST, D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,
				 SAMPLER_SHADOW
				);

		constexpr DXGI_FORMAT intensity_rtv_formats[]
		{
			DXGI_FORMAT_R32G32B32A32_UINT,
			DXGI_FORMAT_R32G32B32A32_FLOAT
		};

		Shader::Create
				(
				 "intensity_test", "./intensity_test.hlsl", SHADER_DOMAIN_OPAQUE,
				 SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
				 SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
				 D3D12_FILTER_MIN_MAG_MIP_POINT,
				 SHADER_SAMPLER_WRAP | SHADER_SAMPLER_NEVER,
				 intensity_rtv_formats, 2, DXGI_FORMAT_D32_FLOAT,
				 D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
				 SAMPLER_SHADOW
				);

		Shader::Create
				(
				 "atlas", "./atlas.hlsl", SHADER_DOMAIN_OPAQUE,
				 SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
				 SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
				 D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				 SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS,
				 &g_default_rtv_format, 1, DXGI_FORMAT_D24_UNORM_S8_UINT,
				 D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
				 SAMPLER_SHADOW
				);
		*/
	}

	void RenderPipeline::PreUpdate(const float& dt) {}

	void RenderPipeline::PreRender(const float& dt)
	{
		//Managers::D3Device::GetInstance().ClearRenderTarget();
	}

	void RenderPipeline::Update(const float& dt) {}

	void RenderPipeline::Render(const float& dt) {}

	void RenderPipeline::FixedUpdate(const float& dt) {}

	void RenderPipeline::PostRender(const float& dt) {}

	void RenderPipeline::PostUpdate(const float& dt) {}

} // namespace Engine::Manager::Graphics

namespace Engine
{
	void PrimitiveTexture::UpdateDescription(
		const Weak<Resources::Texture>& texture,
		const GenericTextureDescription& description)
	{
		if (const Strong<Resources::Texture>& tex = texture.lock())
		{
			tex->UpdateDescription(description);
		}
	}

	void* PrimitiveTexture::GetPrimitiveTexture() const
	{
		return m_texture_;
	}

	void PrimitiveTexture::SetPrimitiveTexture(void* texture)
	{
		if (texture) 
		{
			m_texture_ = texture;
		}
	}

	void* Engine::ComputePrimitiveShader::GetComputePrimitiveShader() const
	{
		return m_shader_;
	}

	void* GraphicPrimitiveShader::GetGraphicPrimitiveShader() const
	{
		return m_shader_;
	}

	void* PrimitivePipeline::GetNativePipeline() const
	{
		return m_pipeline_;
	}

	void PrimitivePipeline::SetPrimitivePipeline(void* pipeline)
	{
		if (pipeline)
		{
			m_pipeline_ = pipeline;	
		}
	}

	void ViewportRenderPrerequisiteTask::SetViewport(const Viewport& viewport)
	{
		m_viewport_ = viewport;
	}

	Viewport ViewportRenderPrerequisiteTask::GetViewport() const
	{
		return m_viewport_;
	}

	void PipelineRenderPrerequisiteTask::SetPrimitivePipeline(PrimitivePipeline* pipeline)
	{
		if (pipeline)
		{
			m_pipeline_ = pipeline;
		}
	}

	PrimitivePipeline* PipelineRenderPrerequisiteTask::GetPrimitivePipeline() const
	{
		return m_pipeline_;
	}

	void ShaderRenderPrerequisiteTask::SetShader(const GraphicPrimitiveShader* shader)
	{
		if (shader)
		{
			m_shader_ = shader;
		}
	}

	void ShaderRenderPrerequisiteTask::SetPipelineSignature(void* signature)
	{
		if (signature)
		{
			m_pipeline_signature_ = signature;
		}
	}

	const GraphicPrimitiveShader* ShaderRenderPrerequisiteTask::GetShader() const
	{
		return m_shader_;
	}

	void* ShaderRenderPrerequisiteTask::GetPipelineSignature() const
	{
		return m_pipeline_signature_;
	}
}
