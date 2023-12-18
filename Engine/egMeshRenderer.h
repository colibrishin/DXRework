#pragma once
#include <functional>
#include "egResourceManager.hpp"

namespace Engine::Components
{
    class MeshRenderer final : public Abstract::Component
    {
    public:
        MeshRenderer(const WeakObject& owner);
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PreRender(const float& dt) override;
        void PostRender(const float& dt) override;
        void Render(const float& dt) override;

        void OnDeserialized() override;

        void SetMesh(const WeakMesh& mesh);
        void AddTexture(const WeakTexture& texture);
        void AddNormalMap(const WeakNormalMap& normal_map);
        void AddVertexShader(const WeakVertexShader& vertex_shader);
        void AddPixelShader(const WeakPixelShader& pixel_shader);

        void RemoveMesh();
        void RemoveTexture(const WeakTexture& texture);
        void RemoveNormalMap(const WeakNormalMap& normal_map);
        void RemoveVertexShader(const WeakVertexShader& vertex_shader);
        void RemovePixelShader(const WeakPixelShader& pixel_shader);

        const WeakMesh& GetMesh() const;

    private:
        SERIALIZER_ACCESS;
        MeshRenderer();

    private:
        template <typename T>
        __forceinline void CycleIterator(std::vector<T>& container, typename std::vector<T>::iterator& iterator)
        {
            if (container.empty())
            {
                return;
            }

            // initial check
            if (iterator == container.end())
            {
                iterator = container.begin();
            }

            ++iterator;

            // after increment check
            if (iterator == container.end())
            {
                iterator = container.begin();
            }
        }

        std::map<std::string, std::set<std::string>> m_resource_map_;

        // Non-serialized
        std::set<StrongResource>      m_resources_;
        WeakMesh                      m_mesh_;
        std::vector<WeakTexture>      m_textures_;
        std::vector<WeakNormalMap>    m_normal_maps_;
        std::vector<WeakVertexShader> m_vertex_shaders_;
        std::vector<WeakPixelShader>  m_pixel_shaders_;
    };
}

BOOST_CLASS_EXPORT_KEY(Engine::Components::MeshRenderer);