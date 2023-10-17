#pragma once
#include "egShader.hpp"

namespace Engine::Graphic
{
	class VertexShader : public Shader<ID3D11VertexShader>
	{
	public:
		VertexShader(const std::wstring& name, const std::filesystem::path& path);
		~VertexShader() override = default;

		ID3D11InputLayout** GetInputLayout() { return m_input_layout_.GetAddressOf(); }

		void GenerateInputLayout();
		void Initialize() override;
		void PreUpdate() override;
		void Update() override;

	private:
		ComPtr<ID3D11InputLayout> m_input_layout_ = nullptr;
		ComPtr<ID3D11VertexShader> m_shader_ = nullptr;
	};

	inline VertexShader::VertexShader(const std::wstring& name, const std::filesystem::path& path) : Shader<ID3D11VertexShader>(name, path)
	{
	}

	inline void VertexShader::GenerateInputLayout()
	{
		
	}

	inline void VertexShader::Initialize()
	{
	}

	inline void VertexShader::PreUpdate()
	{
	}

	inline void VertexShader::Update()
	{
	};
}
