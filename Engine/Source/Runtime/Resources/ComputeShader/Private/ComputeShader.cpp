#include "../Public/ComputeShader.h"

#if WITH_DEBUG
#include "Source/Runtime/Core/Debugger/Public/Debugger.h"
#endif

#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"
#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.h"

namespace Engine::Resources
{
	RESOURCE_SELF_INFER_GETTER_IMPL(ComputeShader);

	void ComputeShader::Dispatch(const GraphicInterfaceContextPrimitive* context, const UINT group_count[3], const Graphics::SBs::LocalParamSB& param) const
	{
		GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();
		gi.Dispatch(context, this, param, group_count);
	}

	ComputePrimitiveShader& ComputeShader::GetComputePrimitiveShader() const
	{
		return *m_primitive_shader_;
	}

	ComputeShader::ComputeShader(
		const std::string&           name,
		const std::filesystem::path& path,
		const std::array<UINT, 3>&   thread
	)
		: Shader
		(name, path, SHADER_DOMAIN_OPAQUE, SHADER_DEPTH_NEVER, SHADER_RASTERIZER_CULL_NONE,
		 SAMPLER_FILTER_MIN_MAG_MIP_POINT, SHADER_SAMPLER_NEVER, GetDefaultRTVFormat())
	{
		SetPath(path);

		for (size_t i = 0; i < 3; ++i) 
		{
			m_thread_[i] = thread[i];
		}
	}

	std::array<UINT, 3> ComputeShader::GetThread() const
	{
		std::array<UINT, 3> thread;
		thread[0] = m_thread_[0];
		thread[1] = m_thread_[1];
		thread[2] = m_thread_[2];
		return thread;
	}

	void ComputeShader::PostUpdate(const float dt) {}

	void ComputeShader::PreUpdate(const float dt) {}

	void ComputeShader::FixedUpdate(const float dt) {}

	void ComputeShader::Update(const float dt) {}

	void ComputeShader::Initialize()
	{
		Shader::Initialize();
	}

	void ComputeShader::Load_INTERNAL()
	{
		m_primitive_shader_ = Unique<ComputePrimitiveShader>(GraphicInterfaceAccessor::GetInterface().GetNewComputePrimitiveShader());
		m_primitive_shader_->Generate(this, GraphicInterfaceAccessor::GetInterface().GetNativePipeline());
		
		loadDerived();
	}

	void ComputeShader::Unload_INTERNAL()
	{
		m_primitive_shader_.reset();

		unloadDerived();
	}

	ComputeShader::ComputeShader()
		: Shader
		  ("", "", SHADER_DOMAIN_OPAQUE, SHADER_DEPTH_NEVER, SHADER_RASTERIZER_CULL_NONE,
		   SAMPLER_FILTER_MIN_MAG_MIP_POINT, SHADER_SAMPLER_NEVER, GetDefaultRTVFormat()),
		  m_thread_{1,} {}
}
