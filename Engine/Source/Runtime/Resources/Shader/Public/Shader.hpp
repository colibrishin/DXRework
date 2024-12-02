#pragma once
#include <filesystem>
#include "Source/Runtime/Abstracts/CoreResource/Public/Resource.h"
#include "Source/Runtime/TypeLibrary/Public/TypeLibrary.h"

#if defined(USE_DX12)
#include <directx/d3d12.h>
#include <wrl/client.h>
#endif

namespace Engine::Resources
{
	class Shader : public Abstracts::Resource
	{
	public:
		RESOURCE_T(RES_T_SHADER)

		Shader(
			const EntityName&             name, const std::filesystem::path& path,
			eShaderDomain                 domain, eShaderDepths              depth,
			eShaderRasterizers            rasterizer, D3D12_FILTER           sampler_filter, eShaderSamplers sampler,
			const DXGI_FORMAT*            rtv_format,
			UINT                          rtv_count     = 1,
			DXGI_FORMAT                   dsv_format    = DXGI_FORMAT_D24_UNORM_S8_UINT,
			D3D_PRIMITIVE_TOPOLOGY        topology      = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
			D3D12_PRIMITIVE_TOPOLOGY_TYPE topology_type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
		);
		~Shader() override = default;

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostUpdate(const float& dt) override;

		eShaderDomain GetDomain() const;

		void SetTopology(D3D_PRIMITIVE_TOPOLOGY topology, D3D12_PRIMITIVE_TOPOLOGY_TYPE type);

		[[nodiscard]] ID3D12PipelineState*        GetPipelineState() const;
		[[nodiscard]] D3D_PRIMITIVE_TOPOLOGY      GetTopology() const;
		[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetShaderHeap() const;

		static boost::weak_ptr<Shader>   Get(const std::string& name);
		static boost::shared_ptr<Shader> Create(
			const std::string&            name,
			const std::filesystem::path&  path,
			eShaderDomain                 domain,
			UINT                          depth,
			UINT                          rasterizer,
			D3D12_FILTER                  filter,
			UINT                          sampler,
			const DXGI_FORMAT*            rtv_format,
			UINT                          rtv_count,
			DXGI_FORMAT                   dsv_format,
			D3D_PRIMITIVE_TOPOLOGY        topology,
			D3D12_PRIMITIVE_TOPOLOGY_TYPE topology_type
		);

	protected:
		void OnSerialized() override;
		void OnDeserialized() override;

		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

		inline static std::vector<std::tuple<eShaderType, std::string, std::string>> s_main_version =
		{
			{SHADER_VERTEX, "vs_main", "vs_5_0"},
			{SHADER_PIXEL, "ps_main", "ps_5_0"},
			{SHADER_GEOMETRY, "gs_main", "gs_5_0"},
			{SHADER_COMPUTE, "cs_main", "cs_5_0"},
			{SHADER_HULL, "hs_main", "hs_5_0"},
			{SHADER_DOMAIN, "ds_main", "ds_5_0"}
		};

		ComPtr<ID3D12PipelineState> m_pipeline_state_;

	private:
		Shader();
		SERIALIZE_DECL

		eShaderDomain              m_domain_;
		bool                       m_depth_flag_;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC m_pipeline_state_desc_;
		D3D12_DEPTH_WRITE_MASK     m_depth_test_;
		D3D12_COMPARISON_FUNC      m_depth_func_;
		D3D12_FILTER               m_smp_filter_;
		D3D12_TEXTURE_ADDRESS_MODE m_smp_address_;
		D3D12_COMPARISON_FUNC      m_smp_func_;
		D3D12_CULL_MODE            m_cull_mode_;
		D3D12_FILL_MODE            m_fill_mode_;

		std::vector<DXGI_FORMAT>                                      m_rtv_formats_;
		DXGI_FORMAT                                                   m_dsv_format_;
		D3D_PRIMITIVE_TOPOLOGY                                        m_topology_;
		D3D12_PRIMITIVE_TOPOLOGY_TYPE                                 m_topology_type_;
		std::vector<std::pair<D3D12_INPUT_ELEMENT_DESC, std::string>> m_il_;
		std::vector<D3D12_INPUT_ELEMENT_DESC>                         m_il_elements_;

		ComPtr<ID3DBlob> m_vs_blob_;
		ComPtr<ID3DBlob> m_ps_blob_;
		ComPtr<ID3DBlob> m_gs_blob_;
		ComPtr<ID3DBlob> m_hs_blob_;
		ComPtr<ID3DBlob> m_ds_blob_;

		ComPtr<ID3D12DescriptorHeap> m_sampler_descriptor_heap_;
	};
} // namespace Engine::Graphic

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Shader);
