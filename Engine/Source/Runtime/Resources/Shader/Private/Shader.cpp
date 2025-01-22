#include "../Public/Shader.h"

#include <ranges>

#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"
#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.h"

#include <magic_enum.hpp>
#include "Shader.generated.h"

namespace Engine::Resources
{
	void Shader::Load_INTERNAL()
	{
		m_primitive_ = Unique<GraphicPrimitiveShader>(GraphicInterfaceAccessor::GetInterface().GetNewGraphicPrimitiveShader());
		m_primitive_->Generate(this, GraphicInterfaceAccessor::GetInterface().GetNativePipeline());
	}

	Shader::Shader(
		const std::filesystem::path& path,
		const eShaderDomain          domain,
		const bool                   depth_enabled,
		const eShaderDepthMode          depth,
		const eShaderDepthFunction     depth_func,
		const eShaderSamplerAddress         sampler_addr,
		const eShaderSamplerFunction        sampler_func,
		const eSamplerFilter sampler_filter,
		const eShaderRasterizerCull rasterizer_cull,
		const eShaderRasterizerDraw rasterizer_draw,
		const std::vector<eFormat>& rtv_formats,
		const eFormat                dsv_format,
		const ePrimitiveTopology     topology,
		const ePrimitiveTopologyType topology_type,
		const eSampler               sampler_slot
	) : 
		Resource(path),
		m_domain_(domain),
		m_depth_enabled_(depth_enabled),
		m_depth_(depth),
		m_depth_func_(depth_func),
		m_sampler_addr_(sampler_addr),
		m_sampler_func_(m_sampler_func_),
		m_sampler_filter_(sampler_filter),
		m_cull_mode_(rasterizer_cull),
		m_draw_mode(rasterizer_draw),
		m_rtv_formats_(rtv_formats),
		m_dsv_format_(dsv_format),
		m_topology_(topology),
		m_topology_type_(topology_type),
		m_sampler_slot_(sampler_slot) {}

	void Shader::Initialize() {}

	void Shader::PreUpdate(const float dt) {}

	void Shader::Update(const float dt) {}

	void Shader::FixedUpdate(const float dt) {}

	void Shader::PostUpdate(const float dt) {}

	void Shader::Unload_INTERNAL()
	{
		m_primitive_.reset();
	}

	void Shader::OnDeserialized()
	{
		Resource::OnDeserialized();
	}

	eShaderDomain Shader::GetDomain() const
	{
		return m_domain_;
	}

	bool Shader::IsDepthEnabled() const
	{
		return m_depth_enabled_;
	}

	eShaderDepthMode Shader::GetDepthMode() const
	{
		return m_depth_;
	}

	eShaderDepthFunction Shader::GetDepthFunction() const
	{
		return m_depth_func_;
	}

	eShaderSamplerAddress Shader::GetSamplerAddressMode() const
	{
		return m_sampler_addr_;
	}

	eShaderSamplerFunction Shader::GetSamplerFunction() const
	{
		return m_sampler_func_;
	}

	eSamplerFilter Shader::GetSamplerFilter() const
	{
		return m_sampler_filter_;
	}

	eShaderRasterizerCull Shader::GetRasterizerCull() const
	{
		return m_cull_mode_;
	}

	eShaderRasterizerDraw Shader::GetRasterizerDraw() const
	{
		return m_draw_mode;
	}

	const std::vector<eFormat>& Shader::GetRTVFormat() const
	{
		return m_rtv_formats_;
	}

	eFormat Shader::GetDSVFormat() const
	{
		return m_dsv_format_;
	}

	ePrimitiveTopology Shader::GetPrimitiveTopology() const
	{
		return m_topology_;
	}

	ePrimitiveTopologyType Shader::GetPrimitiveTopologyType() const
	{
		return m_topology_type_;
	}

	eSampler Shader::GetSampler() const
	{
		return m_sampler_slot_;
	}

	GraphicPrimitiveShader& Shader::GetGraphicPrimitiveShader() const
	{
		return *m_primitive_;
	}

	void Shader::OnSerialized()
	{
		if (exists(GetPath()))
		{
			const std::filesystem::path folder   = GetPrettyTypeName();
			const std::filesystem::path filename = GetPath().filename();
			const std::filesystem::path p        = folder / filename;

			if (!exists(folder))
			{
				create_directory(folder);
			}

			if (GetPath() == p)
			{
				return;
			}

			if (exists(p))
			{
				std::filesystem::remove(p);
			}

			copy_file(GetPath(), p, std::filesystem::copy_options::overwrite_existing);

			SetPath(p);
		}
	}

	Shader::Shader()
		: Resource(""),
		  m_domain_(),
		  m_depth_enabled_(false),
		  m_depth_(),
		  m_depth_func_(),
		  m_sampler_addr_(),
		  m_sampler_filter_(),
		  m_cull_mode_(),
		  m_draw_mode(),
		  m_dsv_format_(),
		  m_topology_(),
		  m_topology_type_(),
		  m_sampler_slot_(),
		  m_depth_flag_(false) { }
} // namespace Engine::Graphic