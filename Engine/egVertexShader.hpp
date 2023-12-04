#pragma once
#include "egShader.hpp"

namespace Engine::Graphic
{
	class VertexShader : public Shader<ID3D11VertexShader>
	{
	public:
		VertexShader(const EntityName& name, const std::filesystem::path& path);
		~VertexShader() override = default;

		ID3D11InputLayout** GetInputLayout() { return m_input_layout_.GetAddressOf(); }

	private:
		ComPtr<ID3D11InputLayout> m_input_layout_ = nullptr;
	};

	inline VertexShader::VertexShader(const EntityName& name,
	                                  const std::filesystem::path& path) : Shader<ID3D11VertexShader>(name, path)
	{
	}
}
