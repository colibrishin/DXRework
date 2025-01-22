#pragma once
#include <wrl/client.h>

#include "Source/Runtime/Resources/Shader/Public/Shader.hpp"
#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"

namespace Engine
{
	struct GRAPHICPRIMITIVESHADERDX12_API DX12GraphicPrimitiveShader : public GraphicPrimitiveShader
	{
		DX12GraphicPrimitiveShader();
		void Generate(const Weak<Resources::Shader>& w_shader, void* pipeline_signature) override;
		[[nodiscard]] ID3D12PipelineState* GetPSO() const;
		[[nodiscard]] ID3D12DescriptorHeap* GetSamplerHeap() const;

	private:
		void ConvertShader(const Resources::Shader* shader);
		static std::vector<std::pair<D3D12_INPUT_ELEMENT_DESC, std::string>> GenerateInputDescription(ID3DBlob* blob);
		static std::vector<std::tuple<eShaderType, std::string, std::string>> s_main_version;

		eShaderDomain              m_domain_ = SHADER_DOMAIN_OPAQUE;
		bool                       m_depth_flag_ = true;
		D3D12_RASTERIZER_DESC      m_rasterizer_desc_{};
		D3D12_DEPTH_STENCIL_DESC   m_depth_stencil_desc_{};
		D3D12_BLEND_DESC           m_blend_desc_{};
		D3D12_DEPTH_WRITE_MASK     m_depth_test_{};
		D3D12_COMPARISON_FUNC      m_depth_func_{};
		D3D12_FILTER               m_smp_filter_{};
		D3D12_TEXTURE_ADDRESS_MODE m_smp_address_{};
		D3D12_COMPARISON_FUNC      m_smp_func_{};
		D3D12_CULL_MODE            m_cull_mode_{};
		D3D12_FILL_MODE            m_fill_mode_{};

		std::vector<DXGI_FORMAT>                                      m_rtv_formats_;
		DXGI_FORMAT                                                   m_dsv_format_ = DXGI_FORMAT_D24_UNORM_S8_UINT;
		D3D_PRIMITIVE_TOPOLOGY                                        m_topology_ = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		D3D12_PRIMITIVE_TOPOLOGY_TYPE                                 m_topology_type_ = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		std::vector<std::pair<D3D12_INPUT_ELEMENT_DESC, std::string>> m_il_;
		std::vector<D3D12_INPUT_ELEMENT_DESC>                         m_il_elements_;

		ComPtr<ID3DBlob> m_vs_blob_ = nullptr;
		ComPtr<ID3DBlob> m_ps_blob_ = nullptr;
		ComPtr<ID3DBlob> m_gs_blob_ = nullptr;
		ComPtr<ID3DBlob> m_hs_blob_ = nullptr;
		ComPtr<ID3DBlob> m_ds_blob_ = nullptr;

		ComPtr<ID3D12DescriptorHeap> m_sampler_descriptor_heap_ = nullptr;
		ComPtr<ID3D12PipelineState> m_pipeline_state_ = nullptr;
	};
}
