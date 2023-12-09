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

		void SetShader(Graphic::IShader* shader);

		void SetWorldMatrix(const TransformBuffer& matrix);
		void SetPerspectiveMatrix(const PerspectiveBuffer& matrix);

		void SetLight(UINT id, const Matrix& world, const Matrix& vp, const Color& color);
		void SetSpecularPower(float power);
		void SetSpecularColor(const Color& color);

		void SetTopology(const D3D11_PRIMITIVE_TOPOLOGY& topology);
		void GetCascadeShadow(const Vector3& light_dir, Vector4 position[], Matrix view[], Matrix projection[], Vector4 clip[]) const;

		void SetWireframeState() const;
		void SetFillState() const;

		void BindLightBuffer();
		void BindVertexBuffer(ID3D11Buffer* buffer);
		void BindIndexBuffer(ID3D11Buffer* buffer);

		void UpdateBuffer(ID3D11Buffer* buffer, const void* data, size_t size);

		void BindResource(eShaderResource resource, ID3D11ShaderResourceView* texture);

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

		ConstantBuffer<PerspectiveBuffer> m_wvp_buffer_data_{};
		ConstantBuffer<TransformBuffer> m_transform_buffer_data_{};
		
		LightBuffer m_light_buffer_{};
		SpecularBuffer m_specular_buffer_{};

		ConstantBuffer<LightBuffer> m_light_buffer_data{};
		ConstantBuffer<SpecularBuffer> m_specular_buffer_data_{};

		std::unordered_map<eShaderType, ID3D11SamplerState*> s_sampler_state_{};
		ComPtr<ID3D11BlendState> m_blend_state_ = nullptr;
		ComPtr<ID3D11RasterizerState> m_rasterizer_state_ = nullptr;
		ComPtr<ID3D11RasterizerState> m_rasterizer_state_wire_ = nullptr;
		ComPtr<ID3D11DepthStencilState> m_depth_stencil_state_ = nullptr;

		std::vector<D3D11_INPUT_ELEMENT_DESC> m_input_element_desc_;
	};
}
