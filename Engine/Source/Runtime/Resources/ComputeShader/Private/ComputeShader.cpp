#include "../Public/ComputeShader.h"

#if WITH_DEBUG
#include "Source/Runtime/Managers/Debugger/Public/Debugger.hpp"
#endif

#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"
#include "Source/Runtime/Managers/ResourceManager/Public/ResourceManager.hpp"

namespace Engine::Resources
{
	RESOURCE_SELF_INFER_GETTER_IMPL(ComputeShader);

	void ComputeShader::Dispatch(const UINT group_count[3], const Graphics::SBs::LocalParamSB& param)
	{
		
		m_primitive_shader_->GetDispatchTask().Dispatch(
			GetSharedPtr<ComputeShader>(),
			param,
			group_count,
			m_unsafe_raw_prerequisites_.data(),
			m_unsafe_raw_prerequisites_.size());

		m_primitive_shader_->GetDispatchTask().Cleanup();
	}

	void ComputeShader::SetPrimitiveShader(ComputePrimitiveShader* shader)
	{
		m_primitive_shader_ = std::unique_ptr<ComputePrimitiveShader>(shader);
	}

	ComputePrimitiveShader& ComputeShader::GetPrimitiveShader() const
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

	void ComputeShader::PostUpdate(const float& dt) {}

	void ComputeShader::PreUpdate(const float& dt) {}

	void ComputeShader::FixedUpdate(const float& dt) {}

	void ComputeShader::Update(const float& dt) {}

	void ComputeShader::Initialize()
	{
		Shader::Initialize();
	}

	void ComputeShader::Load_INTERNAL()
	{
		m_primitive_shader_->Generate(
			GetSharedPtr<ComputeShader>(), 
			Managers::RenderPipeline::GetInstance().GetPrimitivePipeline()->GetNativePipeline());
		
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

namespace Engine 
{
	void* ComputePrimitiveShader::GetComputePrimitiveShader() const
	{
		return m_shader_;
	}

	ComputeDispatchTask& ComputePrimitiveShader::GetDispatchTask() const
	{
		if (m_dispatch_task == nullptr) 
		{
			throw std::runtime_error("Dispatch Task has not been assigned");
		}

		return *m_dispatch_task;
	}
}