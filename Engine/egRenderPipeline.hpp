#pragma once
#include "pch.hpp"

#include <filesystem>

#include "egCommon.hpp"
#include "egShader.hpp"

namespace Engine::Graphic
{
	class RenderPipeline final
	{
	public:
		~RenderPipeline() = default;

		static void Initialize();
		static void SetShader(const std::wstring& name);

		static void SetWorldMatrix(const TransformBuffer& matrix);
		static void SetPerspectiveMatrix(const VPBuffer& matrix);
		static void SetTopology(const D3D11_PRIMITIVE_TOPOLOGY& topology);

		static void BindVertexBuffer(ID3D11Buffer* buffer);
		static void BindIndexBuffer(ID3D11Buffer* buffer);
		static void UpdateBuffer(ID3D11Buffer* buffer, const void* data, size_t size);

		static void BindTexture(ID3D11ShaderResourceView* texture);

		static void DrawIndexed(UINT index_count);

	private:
		RenderPipeline() = default;

		static void CompileShaders();
		static void InitializeSamplers();

		inline static std::unique_ptr<RenderPipeline> s_instance_ = nullptr;

		inline static ComPtr<ID3D11InputLayout> s_input_layout_ = nullptr;

		inline static std::unordered_map<std::wstring, std::shared_ptr<IShader>> s_shader_map_{};

		inline static ConstantBuffer<VPBuffer> s_vp_buffer_data_{};
		inline static ConstantBuffer<TransformBuffer>  s_transform_buffer_data_{};

		inline static std::unordered_map<eShaderType, ComPtr<ID3D11SamplerState>> s_sampler_state_{};
		inline static ComPtr<ID3D11BlendState> s_blend_state_ = nullptr;
		inline static ComPtr<ID3D11RasterizerState> s_rasterizer_state_ = nullptr;

		inline static std::vector<D3D11_INPUT_ELEMENT_DESC> s_input_element_desc_;
	};
}
