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
	ECLASS(resource)
	class ENGINE_SHADER_API Shader : public Abstracts::Resource
	{
		GENERATE_BODY
	public:
		Shader(
			const std::filesystem::path& path,
			const eShaderDomain          domain,
			const eShaderDepths          depth,
			const eShaderRasterizers     rasterizer,
			const eSamplerFilter         sampler_filter,
			const eShaderSamplers        sampler,
			const std::vector<eFormat>&  rtv_formats,
			const eFormat                dsv_format    = TEX_FORMAT_D24_UNORM_S8_UINT,
			const ePrimitiveTopology     topology      = PRIMITIVE_TOPOLOGY_TRIANGLELIST,
			const ePrimitiveTopologyType topology_type = PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			const eSampler               sampler_slot  = SAMPLER_TEXTURE
		);

		~Shader() override = default;

		void Initialize() override;
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostUpdate(const float dt) override;

		[[nodiscard]] eShaderDomain GetDomain() const;
		[[nodiscard]] eShaderDepths GetDepth() const;
		[[nodiscard]] eShaderRasterizers GetRasterizer() const;
		[[nodiscard]] eSamplerFilter GetSamplerFilter() const;
		[[nodiscard]] eShaderSamplers GetShaderSampler() const;
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
		eShaderDomain          m_domain_;
		EPROPERTY()
		eShaderDepths          m_depth_;
		EPROPERTY()
		eShaderRasterizers     m_rasterizer_;
		EPROPERTY()
		eSamplerFilter         m_sampler_filter_;
		EPROPERTY()
		eShaderSamplers        m_sampler_;
		EPROPERTY()
		std::vector<eFormat>   m_rtv_formats_;
		EPROPERTY()
		eFormat                m_dsv_format_;
		EPROPERTY()
		ePrimitiveTopology     m_topology_;
		EPROPERTY()
		ePrimitiveTopologyType m_topology_type_;
		EPROPERTY()
		eSampler               m_sampler_slot_;
		EPROPERTY()
		bool                                    m_depth_flag_;
		std::unique_ptr<GraphicPrimitiveShader> m_primitive_;
	};
} // namespace Engine::Graphic
