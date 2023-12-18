#pragma once
#include <functional>
#include "egType.h"
#include "egResourceManager.hpp"

namespace Engine::Components
{
    class MeshRenderer final : public Abstract::Component
    {
    public:
        INTERNAL_COMP_CHECK_CONSTEXPR(COM_T_MESH_RENDERER)

        MeshRenderer(const WeakObject& owner);
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PreRender(const float& dt) override;
        void PostRender(const float& dt) override;
        void Render(const float& dt) override;

        void OnDeserialized() override;

        void SetMesh(const WeakMesh& mesh);

        template <typename T, typename ResTypeCheck = std::enable_if_t<std::is_base_of_v<Abstract::Resource, T>>>
        void Add (const boost::weak_ptr<T>& resource)
        {
            if (resource.expired())
            {
                return;
            }

            const auto locked = resource.lock();

            if constexpr (which_resource<T>::value == RES_T_SHADER)
            {
                if constexpr (std::is_same_v<T, Graphic::VertexShader>)
                {
                    m_vertex_shaders_.push_back(boost::reinterpret_pointer_cast<Graphic::VertexShader>(locked));
                }
                else if constexpr (std::is_same_v<T, Graphic::PixelShader>)
                {
                    m_pixel_shaders_.push_back(boost::reinterpret_pointer_cast<Graphic::PixelShader>(locked));
                }
            }
            else if constexpr (which_resource<T>::value == RES_T_TEX)
            {
                m_textures_.push_back(boost::reinterpret_pointer_cast<Resources::Texture>(locked));
            }
            else if constexpr (which_resource<T>::value == RES_T_NORMAL)
            {
                m_normal_maps_.push_back(boost::reinterpret_pointer_cast<Resources::NormalMap>(locked));
            }
            else
            {
                static_assert("Unsupported resource type");
            }

            m_resources_.insert(locked);
            m_resource_map_[which_resource<T>::value].insert(locked->GetName());
        }

        template <typename T, typename ResTypeCheck = std::enable_if_t<std::is_base_of_v<Abstract::Resource, T>>>
        void Remove(const boost::weak_ptr<T>& resource)
        {
            if (resource.expired())
            {
                return;
            }

            const auto locked = resource.lock();

            if constexpr (which_resource<T>::value == RES_T_SHADER)
            {
                if constexpr (std::is_same_v<T, Graphic::VertexShader>)
                {
                    m_vertex_shaders_.erase(std::remove_if(m_vertex_shaders_.begin(), m_vertex_shaders_.end(), [&locked](const WeakVertexShader& shader)
                    {
                        return shader.lock() == locked;
                    }), m_vertex_shaders_.end());
                }
                else if constexpr (std::is_same_v<T, Graphic::PixelShader>)
                {
                    m_pixel_shaders_.erase(std::remove_if(m_pixel_shaders_.begin(), m_pixel_shaders_.end(), [&locked](const WeakPixelShader& shader)
                    {
                        return shader.lock() == locked;
                    }), m_pixel_shaders_.end());
                }
            }
            else if constexpr (which_resource<T>::value == RES_T_TEX)
            {
                m_textures_.erase(std::remove_if(m_textures_.begin(), m_textures_.end(), [&locked](const WeakTexture& texture)
                {
                    return texture.lock() == locked;
                }), m_textures_.end());
            }
            else if constexpr (which_resource<T>::value == RES_T_NORMAL)
            {
                m_normal_maps_.erase(std::remove_if(m_normal_maps_.begin(), m_normal_maps_.end(), [&locked](const WeakNormalMap& normal_map)
                {
                    return normal_map.lock() == locked;
                }), m_normal_maps_.end());
            }
            else             
            {
                static_assert("Unsupported resource type");
            }

            m_resources_.erase(locked);
            m_resource_map_[which_resource<T>::value].erase(locked->GetName());
        }

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

        std::map<eResourceType, std::set<std::string>> m_resource_map_;

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