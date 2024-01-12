#pragma once
#include <filesystem>

#include "egCommon.hpp"
#include "egEntity.hpp"
#include "egIShader.h"

namespace Engine::Graphics
{
    template <typename T>
    class Shader : public Graphics::IShader
    {
    public:
        RESOURCE_T(RES_T_SHADER)
        using shaderType = T;

        Shader(const EntityName& name, const std::filesystem::path& path);
        ~Shader() override;

        void Initialize() override;
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PostUpdate(const float& dt) override;

        void SetDomain(const eShaderDomain & domain) override;

        T** GetShader()
        {
            return m_shader_.GetAddressOf();
        }

        void Render(const float& dt) override;
        void PostRender(const float& dt) override;

        RESOURCE_SELF_INFER_GETTER(Shader)
        static inline boost::shared_ptr<Shader> Create(
            const std::string& name, const std::filesystem::path& path, const eShaderDomain& domain);

    protected:
        Shader()
        : IShader("", "") {}

        void Load_INTERNAL() override;
        void Unload_INTERNAL() override;

        void OnDeserialized() override;

    private:
        friend class Serializer;
        friend class boost::serialization::access;

        template <class Archive>
        void serialize(Archive& ar, const unsigned int file_version)
        {
            ar & boost::serialization::base_object<IShader>(*this);
        }

        void SetShaderType() override;

        ComPtr<T> m_shader_;
    };
} // namespace Engine::Graphic

BOOST_CLASS_EXPORT_KEY(Engine::Graphics::Shader<ID3D11VertexShader>);

BOOST_CLASS_EXPORT_KEY(Engine::Graphics::Shader<ID3D11PixelShader>);

BOOST_CLASS_EXPORT_KEY(Engine::Graphics::Shader<ID3D11GeometryShader>);

BOOST_CLASS_EXPORT_KEY(Engine::Graphics::Shader<ID3D11ComputeShader>);

BOOST_CLASS_EXPORT_KEY(Engine::Graphics::Shader<ID3D11HullShader>);

BOOST_CLASS_EXPORT_KEY(Engine::Graphics::Shader<ID3D11DomainShader>);
