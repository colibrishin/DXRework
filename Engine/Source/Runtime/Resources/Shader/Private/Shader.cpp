#include "../Public/Shader.h"
#include "Shader.generated.h"

#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"
#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.h"

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
	)
		: Resource(path),
		  m_domain_(domain),
		  m_depth_enabled_(depth_enabled),
		  m_depth_mode_(depth),
		  m_depth_func_(depth_func),
		  m_sampler_addr_(sampler_addr),
		  m_sampler_func_(sampler_func),
		  m_sampler_filter_(sampler_filter),
		  m_cull_mode_(rasterizer_cull),
		  m_draw_mode_(rasterizer_draw),
		  m_rtv_formats_(rtv_formats),
		  m_dsv_format_(dsv_format),
		  m_topology_(topology),
		  m_topology_type_(topology_type),
		  m_sampler_slot_(sampler_slot)
	{
#if WITH_EDITOR
		UpdateSelected();
#endif
	}

	Shader::Shader(const Shader& other)
		: Resource( other )
	{
		m_domain_         = other.m_domain_;
		m_depth_enabled_  = other.m_depth_enabled_;
		m_depth_mode_     = other.m_depth_mode_;
		m_depth_func_     = other.m_depth_func_;
		m_sampler_addr_   = other.m_sampler_addr_;
		m_sampler_func_   = other.m_sampler_func_;
		m_sampler_filter_ = other.m_sampler_filter_;
		m_cull_mode_      = other.m_cull_mode_;
		m_draw_mode_      = other.m_draw_mode_;
		m_rtv_formats_    = other.m_rtv_formats_;
		m_dsv_format_     = other.m_dsv_format_;
		m_topology_       = other.m_topology_;
		m_topology_type_  = other.m_topology_type_;
		m_sampler_slot_   = other.m_sampler_slot_;

#if WITH_EDITOR
		UpdateSelected();
#endif
	}

	Shader& Shader::operator=(const Shader& other)
	{
		m_domain_ = other.m_domain_;
		m_depth_enabled_ = other.m_depth_enabled_;
		m_depth_mode_ = other.m_depth_mode_;
		m_depth_func_ = other.m_depth_func_;
		m_sampler_addr_ = other.m_sampler_addr_ ;
		m_sampler_func_ = other.m_sampler_func_;
		m_sampler_filter_ = other.m_sampler_filter_;
		m_cull_mode_ = other.m_cull_mode_;
		m_draw_mode_ = other.m_draw_mode_;
		m_rtv_formats_ = other.m_rtv_formats_;
		m_dsv_format_ = other.m_dsv_format_;
		m_topology_ = other.m_topology_;
		m_topology_type_ = other.m_topology_type_;
		m_sampler_slot_ = other.m_sampler_slot_;

#if WITH_EDITOR
		UpdateSelected();
#endif
		return *this;
	}

#if WITH_EDITOR
	void Shader::OnUIUpdate(UIContext* const parent, const float dt)
	{
		if ( parent )
		{
			Resource::OnUIUpdate( parent, dt );

#define ENUM_COMBOBOX(NAME, THIS_VAR, ENUM_TYPE, ENUM_ARR) \
	(*parent |= ui.NewCombobox( { NAME, &##THIS_VAR##selected_, ENUM_ARR##.data(), ENUM_ARR##.size() } )).SetFunction( [this]() {\
		(THIS_VAR) = RecastNonlinearEnum<##ENUM_TYPE##>( ENUM_ARR, THIS_VAR##selected_ ); });

			UIInterface& ui = UIInterfaceAccessor::GetInterface();
			ENUM_COMBOBOX( "Shader Domain", m_domain_, eShaderDomain, domain_enums );
			*parent |= ui.NewCheckbox( {"Depth Enabled", m_depth_enabled_} );
			ENUM_COMBOBOX( "Depth Mode", m_depth_mode_, eShaderDepthMode, depth_mode_enums );
			ENUM_COMBOBOX( "Depth Function", m_depth_func_, eShaderDepthFunction, depth_function_enums );
			ENUM_COMBOBOX( "Cull Mode", m_cull_mode_, eShaderRasterizerCull, rasterizer_cull_enums );
			ENUM_COMBOBOX( "Draw Mode", m_draw_mode_, eShaderRasterizerDraw, rasterizer_draw_enums );
			ENUM_COMBOBOX( "Sampler Filter", m_sampler_filter_, eSamplerFilter, filter_enums );
			ENUM_COMBOBOX( "Sampler Address Mode", m_sampler_addr_, eShaderSamplerAddress, sampler_addr_enums );
			ENUM_COMBOBOX( "Sampler Function", m_sampler_func_, eShaderSamplerFunction, sampler_func_enums );

			{
				*parent += ui.NewListBox({ "RenderTarget Format", -1, 0 });

				static std::vector<std::string> render_target_label {
					"Render Target 0",
					"Render Target 1",
					"Render Target 2",
					"Render Target 3",
					"Render Target 4",
					"Render Target 5",
					"Render Target 6",
					"Render Target 7"
				};
				
				for (size_t i = 0; i < m_rtv_formats_.size(); ++i)
				{
					(*parent |= ui.NewCombobox( { render_target_label[i], &m_rtv_formats_selected_[i], format_enums.data(), format_enums.size() } )).SetFunction( [this, i]()
					{
						m_rtv_formats_[i] = RecastNonlinearEnum<eFormat>( format_enums, m_rtv_formats_selected_[i] );
					} );
				}
				--*parent;

				(*parent |= ui.NewButton( { "Add Render Target" } )).SetFunction( [this]()
					{
						const eFormat default_format = GetDefaultRTVFormat().front();
						m_rtv_formats_.push_back( default_format );
						m_rtv_formats_selected_.push_back( default_format );
					} );
			}

			ENUM_COMBOBOX( "Depth/Stencil Format", m_dsv_format_, eFormat, format_enums );
			ENUM_COMBOBOX( "Primitive Topology", m_topology_, ePrimitiveTopology, primitive_topology_enum );
			ENUM_COMBOBOX( "Primitive Topology Type", m_topology_type_, ePrimitiveTopologyType, primitive_topology_type_enum );
			ENUM_COMBOBOX( "Sampler Slot", m_sampler_slot_, eSampler, sampler_slot_enum );
			
#undef ENUM_COMBOBOX
		}
	}
#endif
	
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
		return m_depth_mode_;
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
		return m_draw_mode_;
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
		  m_depth_mode_(),
		  m_depth_func_(),
		  m_sampler_addr_(),
		  m_sampler_func_(),
		  m_sampler_filter_(),
		  m_cull_mode_(),
		  m_draw_mode_(),
		  m_dsv_format_(),
		  m_topology_(),
		  m_topology_type_(),
		  m_sampler_slot_() {}

#if WITH_EDITOR
	void Shader::UpdateSelected()
	{
#define CAST_ENUM(THIS_VAR)\
		if (const auto& val = magic_enum::enum_index(THIS_VAR); val.has_value())\
		{ THIS_VAR##selected_ = static_cast<int>(val.value()); }

		CAST_ENUM( m_domain_ )
		CAST_ENUM( m_depth_mode_ )
		CAST_ENUM( m_depth_func_ )
		CAST_ENUM( m_sampler_func_ )
		CAST_ENUM( m_sampler_filter_ )
		CAST_ENUM( m_cull_mode_ )
		CAST_ENUM( m_dsv_format_ )
		CAST_ENUM( m_topology_ )
		CAST_ENUM( m_topology_type_ )
		CAST_ENUM( m_sampler_slot_ )
#undef CAST_ENUM
		
		m_rtv_formats_selected_.resize(m_rtv_formats_.size());
		for ( size_t i = 0; i < m_rtv_formats_selected_.size(); ++i )
		{
			if ( const auto& val = magic_enum::enum_index(m_rtv_formats_[i]);
				 val.has_value() )
			{
				m_rtv_formats_selected_[i] = static_cast<int>(val.value());
			} 
		}
	}
#endif
} // namespace Engine::Graphic