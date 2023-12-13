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
		Shader(const EntityName& name, const std::filesystem::path& path);
		~Shader() override;

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void FixedUpdate(const float& dt) override;

		T** GetShader() { return m_shader_.GetAddressOf(); }
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;

		TypeName GetVirtualTypeName() const final;

	protected:
		Shader() : IShader("", "") {}
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

		void OnDeserialized() override;

	private:
		friend class Engine::Serializer;
		friend class boost::serialization::access;

		template<class Archive>
		void serialize(Archive& ar, const unsigned int file_version)
		{
			ar & boost::serialization::base_object<Engine::Graphic::IShader>(*this);
		}

		void SetShaderType() override;

		ComPtr<T> m_shader_;

	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Graphic::Shader<ID3D11VertexShader>);
BOOST_CLASS_EXPORT_KEY(Engine::Graphic::Shader<ID3D11PixelShader>);
BOOST_CLASS_EXPORT_KEY(Engine::Graphic::Shader<ID3D11GeometryShader>);
BOOST_CLASS_EXPORT_KEY(Engine::Graphic::Shader<ID3D11ComputeShader>);
BOOST_CLASS_EXPORT_KEY(Engine::Graphic::Shader<ID3D11HullShader>);
BOOST_CLASS_EXPORT_KEY(Engine::Graphic::Shader<ID3D11DomainShader>);
