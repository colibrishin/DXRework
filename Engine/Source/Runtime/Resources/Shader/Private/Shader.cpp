#include "../Public/Shader.hpp"

#include <ranges>

#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"
#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.hpp"

namespace Engine::Resources
{
	void Shader::Load_INTERNAL()
	{
		m_primitive_ = std::unique_ptr<GraphicPrimitiveShader>();
		m_primitive_->Generate(GetSharedPtr<Shader>(),
			g_graphic_interface.GetInterface().GetNativePipeline());
	}

	Shader::Shader(
		const EntityName&            name,
		const std::filesystem::path& path,
		const eShaderDomain          domain,
		const eShaderDepths          depth,
		const eShaderRasterizers     rasterizer,
		const eSamplerFilter         sampler_filter,
		const eShaderSamplers        sampler,
		const std::vector<eFormat>&  rtv_formats,
		const eFormat                dsv_format,
		const ePrimitiveTopology     topology,
		const ePrimitiveTopologyType topology_type,
		const eSampler               sampler_slot  
	)
		: Resource(path, RES_T_SHADER),
		m_domain_(domain),
		m_depth_(depth),
		m_rasterizer_(rasterizer),
		m_sampler_filter_(sampler_filter),
		m_sampler_(sampler),
		m_rtv_formats_(rtv_formats),
		m_dsv_format_(dsv_format),
		m_topology_(topology),
		m_topology_type_(topology_type),
		m_sampler_slot_(sampler_slot)
	{
		SetName(name);
	}

	void Shader::Initialize() {}

	void Shader::PreUpdate(const float& dt) {}

	void Shader::Update(const float& dt) {}

	void Shader::FixedUpdate(const float& dt) {}

	void Shader::PostUpdate(const float& dt) {}

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

	eShaderDepths Shader::GetDepth() const
	{
		return m_depth_;
	}

	eShaderRasterizers Shader::GetRasterizer() const
	{
		return m_rasterizer_;
	}

	eSamplerFilter Shader::GetSamplerFilter() const
	{
		return m_sampler_filter_;
	}

	eShaderSamplers Shader::GetShaderSampler() const
	{
		return m_sampler_;
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

	GraphicPrimitiveShader* Shader::GetPrimitiveShader() const
	{
		return m_primitive_.get();
	}

	boost::weak_ptr<Shader> Shader::Get(const std::string& name)
	{
		return Managers::ResourceManager::GetInstance().GetResource<Shader>(name);
	}

	boost::shared_ptr<Shader> Shader::Create(
			const EntityName&            name,
			const std::filesystem::path& path,
			const eShaderDomain          domain,
			const eShaderDepths          depth,
			const eShaderRasterizers     rasterizer,
			const eSamplerFilter         sampler_filter,
			const eShaderSamplers        sampler,
			const std::vector<eFormat>&  rtv_formats,
			const eFormat                dsv_format,
			const ePrimitiveTopology     topology,
			const ePrimitiveTopologyType topology_type,
			const eSampler               sampler_slot
	)
	{
		if (const auto pcheck = Managers::ResourceManager::GetInstance().GetResourceByRawPath<Shader>
					(path).lock();
			const auto ncheck = Managers::ResourceManager::GetInstance().GetResource<Shader>(name).lock())
		{
			return ncheck;
		}

		const auto obj = boost::make_shared<Shader>
				(
				 name, path, domain, depth, rasterizer, sampler_filter, 
				 sampler, rtv_formats, dsv_format, topology,
				 topology_type, sampler_slot
				);

		Managers::ResourceManager::GetInstance().AddResource(name, obj);
		return obj;
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
		: Resource("", RES_T_SHADER),
		  m_domain_(),
		  m_depth_(0),
		  m_rasterizer_(0),
		  m_sampler_filter_(),
		  m_sampler_(0),
		  m_dsv_format_(),
		  m_topology_(),
		  m_topology_type_(),
		  m_sampler_slot_(),
		  m_depth_flag_(false) { }
} // namespace Engine::Graphic

namespace Engine 
{
	void GraphicPrimitiveShader::SetNativeShader(void* shader) 
	{
		if (shader) 
		{
			m_shader_ = shader;
		}
	}

	void GraphicPrimitiveShader::SetNativeSampler(void* sampler) 
	{
		if (sampler) 
		{
			m_sampler_ = sampler;
		}
	}
}