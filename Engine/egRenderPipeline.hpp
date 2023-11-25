#pragma once
#include <filesystem>
#include <BufferHelpers.h>

#include "egDXCommon.h"
#include "egCommon.hpp"
#include "egD3Device.hpp"

namespace Engine::Graphic
{
	class IShader;
}

namespace Engine::Manager::Graphics
{
	using Microsoft::WRL::ComPtr;
	using DirectX::ConstantBuffer;

	class RenderPipeline final : public Abstract::Singleton<RenderPipeline>
	{
	public:
		RenderPipeline(SINGLETON_LOCK_TOKEN) : Singleton() {}
		~RenderPipeline() override = default;

		void Initialize() override;
		void PreRender(const float& dt) override;

		static void SetShader(Graphic::IShader* shader);

		void SetWorldMatrix(const TransformBuffer& matrix);
		void SetPerspectiveMatrix(const VPBuffer& matrix);

		void SetLightPosition(UINT id, const Vector3& position);
		void SetLightColor(UINT id, const Vector4& color);
		void SetSpecularPower(float power);
		void SetSpecularColor(const Color& color);

		static void SetTopology(const D3D11_PRIMITIVE_TOPOLOGY& topology);

		void SetWireframeState() const;
		void SetFillState() const;

		void BindLightBuffers();
		static void BindVertexBuffer(ID3D11Buffer* buffer);
		static void BindIndexBuffer(ID3D11Buffer* buffer);
		static void UpdateBuffer(ID3D11Buffer* buffer, const void* data, size_t size);

		static void BindResource(eShaderResource resource, ID3D11ShaderResourceView* texture);

		static void DrawIndexed(UINT index_count);

	private:
		friend class ToolkitAPI;
		friend class D3Device;

		void PrecompileShaders();
		void InitializeSamplers();

		void PreUpdate(const float& dt) override {}
		void Update(const float& dt) override {}
		void Render(const float& dt) override {}
		void FixedUpdate(const float& dt) override {}

	private:
		ComPtr<ID3D11InputLayout> m_input_layout_ = nullptr;

		ConstantBuffer<VPBuffer> m_vp_buffer_data_{};
		ConstantBuffer<TransformBuffer> m_transform_buffer_data_{};
		
		LightPositionBuffer m_light_position_buffer_{};
		LightColorBuffer m_light_color_buffer_{};
		SpecularBuffer m_specular_buffer_{};

		ConstantBuffer<LightPositionBuffer> m_light_position_buffer_data_{};
		ConstantBuffer<LightColorBuffer> m_light_color_buffer_data_{};
		ConstantBuffer<SpecularBuffer> m_specular_buffer_data_{};

		std::unordered_map<eShaderType, ID3D11SamplerState*> s_sampler_state_{};
		ComPtr<ID3D11BlendState> m_blend_state_ = nullptr;
		ComPtr<ID3D11RasterizerState> m_rasterizer_state_ = nullptr;
		ComPtr<ID3D11RasterizerState> m_rasterizer_state_wire_ = nullptr;
		ComPtr<ID3D11DepthStencilState> m_depth_stencil_state_ = nullptr;

		std::vector<D3D11_INPUT_ELEMENT_DESC> m_input_element_desc_;
	};
}
