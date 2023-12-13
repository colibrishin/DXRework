#include "pch.hpp"
#include "egShader.hpp"
#include "egManagerHelper.hpp"
#include "egRenderPipeline.hpp"

BOOST_CLASS_EXPORT_IMPLEMENT(Engine::Graphic::Shader<ID3D11VertexShader>);
BOOST_CLASS_EXPORT_IMPLEMENT(Engine::Graphic::Shader<ID3D11PixelShader>);
BOOST_CLASS_EXPORT_IMPLEMENT(Engine::Graphic::Shader<ID3D11GeometryShader>);
BOOST_CLASS_EXPORT_IMPLEMENT(Engine::Graphic::Shader<ID3D11ComputeShader>);
BOOST_CLASS_EXPORT_IMPLEMENT(Engine::Graphic::Shader<ID3D11HullShader>);
BOOST_CLASS_EXPORT_IMPLEMENT(Engine::Graphic::Shader<ID3D11DomainShader>);

namespace Engine::Graphic
{
	// explicit template instantiation
	template class Shader<ID3D11VertexShader>;
	template class Shader<ID3D11PixelShader>;
	template class Shader<ID3D11GeometryShader>;
	template class Shader<ID3D11ComputeShader>;
	template class Shader<ID3D11HullShader>;
	template class Shader<ID3D11DomainShader>;

	template <typename T>
	void Shader<T>::Load_INTERNAL()
	{
		GetD3Device().CreateShader(GetPath(), this);
	}

	template <typename T>
	Shader<T>::Shader(const EntityName& name, const std::filesystem::path& path) : IShader(name, path)
	{
		Shader<T>::SetShaderType();
	}

	template <typename T>
	Shader<T>::~Shader()
	{
		if (m_shader_)
		{
			m_shader_.Reset();
		}
	}

	template <typename T>
	void Shader<T>::Initialize()
	{
		IShader::Initialize();
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
	void Shader<T>::PreRender(const float& dt)
	{
	}

	template <typename T>
	void Shader<T>::Unload_INTERNAL()
	{
		m_shader_.Reset();
	}

	template <typename T>
	void Shader<T>::OnDeserialized()
	{
		IShader::OnDeserialized();
		SetShaderType();
	}

	template <typename T>
	void Shader<T>::Render(const float& dt)
	{
		GetRenderPipeline().SetShader(this);
	}

	template <typename T>
	void Shader<T>::PostRender(const float& dt)
	{
	}

	template <typename T>
	TypeName Shader<T>::GetVirtualTypeName() const
	{
		return typeid(Shader<T>).name();
	}
}
