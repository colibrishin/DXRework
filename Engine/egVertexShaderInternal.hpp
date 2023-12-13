#pragma once
#include "egShader.hpp"

namespace Engine::Graphic
{
	class VertexShaderInternal : public Shader<ID3D11VertexShader>
	{
	public:
		VertexShaderInternal(const EntityName& name, const std::filesystem::path& path);
		~VertexShaderInternal() override = default;

		ID3D11InputLayout** GetInputLayout() { return m_input_layout_.GetAddressOf(); }

		void Render(const float& dt) override;

	protected:
		VertexShaderInternal() : Shader<ID3D11VertexShader>("", {}) {}

	private:
		SERIALIZER_ACCESS

		D3D11_PRIMITIVE_TOPOLOGY m_topology_ = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		ComPtr<ID3D11InputLayout> m_input_layout_ = nullptr;
	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Graphic::VertexShaderInternal)