#pragma once
#include <filesystem>
#include <magic_enum.hpp>
#include <memory>

#include "ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"

#include "ObjectBase/Public/ObjectBase.h"
#include "ResourceManager/Public/ResourceManager.h"
#include "Source/Runtime/Core/Resource/Public/Resource.h"

#include "Shader.generated.h"


namespace Engine 
{
	struct GraphicPrimitiveShader;
}

template <typename Enum>
constexpr auto CStrEnumStrings()
{
	constexpr auto enum_val = magic_enum::enum_names<Enum>();
	std::array<const char*, enum_val.size()> ret{};
	for (size_t i = 0; i < enum_val.size(); ++i)
	{
		ret[i] = enum_val[i].data();
	}
	return ret;
}

template <typename Enum>
Enum RecastNonlinearEnum(const auto& cstr_array, size_t value)
{
	if (const auto format_validity = magic_enum::enum_cast<Enum>(cstr_array[value]);
		format_validity.has_value())
	{
		return format_validity.value();
	}

	return static_cast<Enum>(0);
}

namespace Engine::Resources
{
	ECLASS(resource, serialize)
	class ENGINE_SHADER_API Shader : public Abstracts::Resource
	{
		GENERATE_BODY
		
		static constexpr auto domain_enums = CStrEnumStrings<eShaderDomain>();
		static constexpr auto depth_mode_enums = CStrEnumStrings<eShaderDepthMode>();
		static constexpr auto depth_function_enums = CStrEnumStrings<eShaderDepthFunction>();
		static constexpr auto rasterizer_cull_enums = CStrEnumStrings<eShaderRasterizerCull>();
		static constexpr auto rasterizer_draw_enums = CStrEnumStrings<eShaderRasterizerDraw>();
		static constexpr auto filter_enums = CStrEnumStrings<eSamplerFilter>();
		static constexpr auto sampler_addr_enums = CStrEnumStrings<eShaderSamplerAddress>();
		static constexpr auto sampler_func_enums = CStrEnumStrings<eShaderSamplerFunction>();
		static constexpr auto format_enums = CStrEnumStrings<eFormat>();
		static constexpr auto primitive_topology_enum = CStrEnumStrings<ePrimitiveTopology>();
		static constexpr auto primitive_topology_type_enum = CStrEnumStrings<ePrimitiveTopologyType>();
		static constexpr auto sampler_slot_enum = CStrEnumStrings<eSampler>();
		
	public:
		Shader(
			const std::filesystem::path& path,
			eShaderDomain                domain,
			bool                         depth_enabled,
			eShaderDepthMode             depth,
			eShaderDepthFunction         depth_func,
			eShaderSamplerAddress        sampler_addr,
			eShaderSamplerFunction       sampler_func,
			eSamplerFilter               sampler_filter,
			eShaderRasterizerCull        rasterizer_cull,
			eShaderRasterizerDraw        rasterizer_draw,
			const std::vector<eFormat>&  rtv_formats,
			eFormat                      dsv_format    = TEX_FORMAT_D24_UNORM_S8_UINT,
			ePrimitiveTopology           topology      = PRIMITIVE_TOPOLOGY_TRIANGLELIST,
			ePrimitiveTopologyType       topology_type = PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			eSampler                     sampler_slot  = SAMPLER_TEXTURE
		);

		Shader(const Shader& other);
		Shader& operator=(const Shader& other);

		~Shader() override = default;

#if WITH_EDITOR
		void OnUIUpdate(UIContext* const parent, const float dt) override;
#endif
		
		void Initialize() override;
		void PreUpdate(float dt) override;
		void Update(float dt) override;
		void FixedUpdate(float dt) override;
		void PostUpdate(float dt) override;

		[[nodiscard]] eShaderDomain GetDomain() const;
		[[nodiscard]] bool IsDepthEnabled() const;
		[[nodiscard]] eShaderDepthMode GetDepthMode() const;
		[[nodiscard]] eShaderDepthFunction GetDepthFunction() const;
		[[nodiscard]] eShaderSamplerAddress GetSamplerAddressMode() const;
		[[nodiscard]] eShaderSamplerFunction GetSamplerFunction() const;
		[[nodiscard]] eSamplerFilter GetSamplerFilter() const;
		[[nodiscard]] eShaderRasterizerCull GetRasterizerCull() const;
		[[nodiscard]] eShaderRasterizerDraw GetRasterizerDraw() const;
		[[nodiscard]] const std::vector<eFormat>& GetRTVFormat() const;
		[[nodiscard]] eFormat GetDSVFormat() const;
		[[nodiscard]] ePrimitiveTopology GetPrimitiveTopology() const;
		[[nodiscard]] ePrimitiveTopologyType GetPrimitiveTopologyType() const;
		[[nodiscard]] eSampler GetSampler() const;
		[[nodiscard]] GraphicPrimitiveShader& GetGraphicPrimitiveShader() const;

	protected:
		void OnSerialized() override;
		void OnDeserialized() override;

		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		Shader();

		EPROPERTY()
		eShaderDomain m_domain_;
		EPROPERTY()
		bool m_depth_enabled_;
		EPROPERTY()
		eShaderDepthMode m_depth_mode_;
		EPROPERTY()
		eShaderDepthFunction m_depth_func_;
		EPROPERTY()
		eShaderSamplerAddress m_sampler_addr_;
		EPROPERTY()
		eShaderSamplerFunction m_sampler_func_;
		EPROPERTY()
		eSamplerFilter m_sampler_filter_;
		EPROPERTY()
		eShaderRasterizerCull m_cull_mode_;
		EPROPERTY()
		eShaderRasterizerDraw m_draw_mode_;
		EPROPERTY()
		std::vector<eFormat> m_rtv_formats_;
		EPROPERTY()
		eFormat m_dsv_format_;
		EPROPERTY()
		ePrimitiveTopology m_topology_;
		EPROPERTY()
		ePrimitiveTopologyType m_topology_type_;
		EPROPERTY()
		eSampler m_sampler_slot_;

#if WITH_EDITOR
		inline void UpdateRtvFormats();
		
		int m_domain_selected_ = 0;
		int m_depth_mode_selected_ = 0;
		int m_depth_func_selected_ = 0;
		int m_sampler_addr_selected_ = 0;
		int m_sampler_func_selected_ = 0;
		int m_sampler_filter_selected_ = 0;
		int m_cull_mode_selected_ = 0;
		int m_draw_mode_selected_ = 0;
		std::vector<int> m_rtv_formats_selected_;
		int m_dsv_format_selected_ = 0;
		int m_topology_selected_ = 0;
		int m_topology_type_selected_ = 0;
		int m_sampler_slot_selected_ = 0;
#endif
		
		std::unique_ptr<GraphicPrimitiveShader> m_primitive_;
	};
} // namespace Engine::Graphic
