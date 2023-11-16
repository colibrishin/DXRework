#pragma once
#include <filesystem>

#include "egCommon.hpp"
#include "egEntity.hpp"
#include "egIShader.hpp"

namespace Engine::Graphic
{
	template <typename T>
	class Shader : public IShader
	{
	public:
		Shader(const std::wstring& name, const std::filesystem::path& path);
		~Shader() override = default;

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void SetShaderType() override;
		void PreRender(const float dt) override;
		void FixedUpdate(const float& dt) override;

		T** GetShader() { return m_shader_.GetAddressOf(); }
		void Render(const float dt) override;

	protected:
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		ComPtr<T> m_shader_;
	};

	template <typename T>
	Shader<T>::Shader(const std::wstring& name, const std::filesystem::path& path) : IShader(name, path)
	{
		Shader<T>::SetShaderType();
	}

	template <typename T>
	void Shader<T>::Initialize()
	{
	}

	template <typename T>
	void Shader<T>::PreUpdate(const float& dt)
	{
	}

	template <typename T>
	void Shader<T>::Update(const float& dt)
	{
	}

	template <typename T>
	void Shader<T>::FixedUpdate(const float& dt)
	{
	}

	template <typename T>
	void Shader<T>::SetShaderType()
	{
		if constexpr (std::is_same_v<T, ID3D11VertexShader>)
		{
			m_type_ = SHADER_VERTEX;
		}
		else if constexpr (std::is_same_v<T, ID3D11PixelShader>)
		{
			m_type_ = SHADER_PIXEL;
		}
		else if constexpr (std::is_same_v<T, ID3D11GeometryShader>)
		{
			m_type_ = SHADER_GEOMETRY;
		}
		else if constexpr (std::is_same_v<T, ID3D11HullShader>)
		{
			m_type_ = SHADER_HULL;
		}
		else if constexpr (std::is_same_v<T, ID3D11DomainShader>)
		{
			m_type_ = SHADER_DOMAIN;
		}
		else if constexpr (std::is_same_v<T, ID3D11ComputeShader>)
		{
			m_type_ = SHADER_COMPUTE;
		}
	}

	template <typename T>
	void Shader<T>::PreRender(const float dt)
	{
	}

	template <typename T>
	void Shader<T>::Unload_INTERNAL()
	{
		m_shader_->Release();
	}

	template <typename T>
	void Shader<T>::Render(const float dt)
	{
		IShader::Render(dt);
	}
}
