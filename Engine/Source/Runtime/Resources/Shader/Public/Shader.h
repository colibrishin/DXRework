#pragma once
#include <filesystem>
#include <memory>

#include "ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"

#include "Source/Runtime/Core/Resource/Public/Resource.h"
#include "Source/Runtime/Managers/RenderPipeline/Public/RenderType.h"
#include "ResourceManager/Public/ResourceManager.h"

#include "Shader.generated.h"

namespace Engine 
{
	struct GraphicPrimitiveShader;
}

namespace Engine::Resources
{
	ECLASS(resource, serialize)
	class ENGINE_SHADER_API Shader : public Abstracts::Resource
	{
		GENERATE_BODY
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

		~Shader() override = default;

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
		eShaderDepthMode m_depth_;
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
		eShaderRasterizerDraw m_draw_mode;
		EPROPERTY()
		std::vector<eFormat> m_rtv_formats_;
		EPROPERTY()
		eFormat m_dsv_format_;
		EPROPERTY()
		ePrimitiveTopology m_topology_;
		EPROPERTY()
		ePrimitiveTopologyType m_topology_type_;
		EPROPERTY()
		eSampler                                m_sampler_slot_;
		std::unique_ptr<GraphicPrimitiveShader> m_primitive_;
	};
} // namespace Engine::Graphic
