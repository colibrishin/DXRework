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

	protected:
		VertexShaderInternal() : Shader<ID3D11VertexShader>("", {}) {}

	private:
		SERIALIZER_ACCESS

		ComPtr<ID3D11InputLayout> m_input_layout_ = nullptr;
	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Graphic::VertexShaderInternal)