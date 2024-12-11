#pragma once
#include <filesystem>
#include <memory>

#include "ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/Core/Resource/Public/Resource.h"

namespace Engine
{
	struct SHADER_API GraphicPrimitiveShader
	{
	public:
		virtual             ~GraphicPrimitiveShader() = default;
		virtual void        Generate(const Weak<Resources::Shader>& shader, void* pipeline_signature) = 0;
		[[nodiscard]] void* GetGraphicPrimitiveShader() const;

	private:
		void* m_shader_ = nullptr;
	};
}

namespace Engine::Resources
{
	class SHADER_API Shader : public Abstracts::Resource
	{
	public:
		RESOURCE_T(RES_T_SHADER)

		Shader(
			const EntityName&            name,
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
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostUpdate(const float& dt) override;

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
		[[nodiscard]] GraphicPrimitiveShader* GetPrimitiveShader() const;

		static boost::weak_ptr<Shader>   Get(const std::string& name);
		static boost::shared_ptr<Shader> Create(
			const EntityName&            name,
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

	protected:
		void OnSerialized() override;
		void OnDeserialized() override;

		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		Shader();

		eShaderDomain          m_domain_;
		eShaderDepths          m_depth_;
		eShaderRasterizers     m_rasterizer_;
		eSamplerFilter         m_sampler_filter_;
		eShaderSamplers        m_sampler_;
		std::vector<eFormat>   m_rtv_formats_;
		eFormat                m_dsv_format_;
		ePrimitiveTopology     m_topology_;
		ePrimitiveTopologyType m_topology_type_;
		eSampler               m_sampler_slot_;

		bool                                    m_depth_flag_;
		std::unique_ptr<GraphicPrimitiveShader> m_primitive_;
	};
} // namespace Engine::Graphic
