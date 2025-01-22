#pragma once
#include <boost/type.hpp>
#include <boost/mpl/find_if.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/copy.hpp>
#include "Resource/Public/Resource.h"

#include "AtlasAnimationTexture.h"
#include "ShapeInstanceBindFlag.h"
#include "Shader.h"
#include "Shape.h"
#include "ShapeInstance.generated.h"

namespace Engine::Resources
{
    ECLASS(resource, serialize)
    class ENGINE_SHAPEINSTANCE_API ShapeInstance : public Abstracts::Resource
    {
        GENERATE_BODY
    public:
        ShapeInstance(const std::vector<Weak<Resource>>& resources);
        
        // todo: compute shader also can be pass-through.
        
        typedef fast_pool_unordered_map<const eShaderDomain, Strong<Shader>> ShaderMap;
        typedef std::array<Strong<Texture>, BIND_SLOT_END> TextureArray;
        typedef fast_pool_unordered_map<ResourceType, aligned_vector<Strong<Resource>>> ResourceMap;
        
        [[nodiscard]] bool IsRenderDomain(eShaderDomain domain) const noexcept;

        typedef boost::mpl::vector<Shader, Texture, Shape> ExplicitAllowedType;

        template <typename T> requires (!std::is_same_v<typename boost::mpl::find_if<ExplicitAllowedType, boost::is_base_of<boost::mpl::_1, T>>::type, boost::mpl::end<AllowedType>::type>)
        void AddResource(const Weak<T>& resource)
        {
            if (const Strong<T>& locked = resource.lock())
            {
                if constexpr (std::is_base_of_v<Shader, T>)
                {
                    addShaderImpl(locked);
                }
                else if constexpr (std::is_base_of_v<Texture, T>)
                {
                    addTextureImpl(locked);
                }
                else if constexpr (std::is_base_of_v<Shape, T>)
                {
                    addShapeImpl(locked);
                }
            }
        }

        void AddResource(const Weak<Resource>& resource)
        {
            if (const Strong<Resource>& locked = resource.lock())
            {
                if (Shader::StaticIsBaseOf(locked->GetTypeHash()))
                {
                    addShaderImpl(locked->GetSharedPtr<Shader>());
                }
                else if (Texture::StaticIsBaseOf(locked->GetTypeHash()))
                {
                    addTextureImpl(locked->GetSharedPtr<Texture>());
                }
                else if (Shape::StaticIsBaseOf(locked->GetTypeHash()))
                {
                    addShapeImpl(locked->GetSharedPtr<Shape>());
                }
            }
        }
        
#if WITH_EDITOR
        void OnUIUpdate(UIContext* const parent, const float dt) override;
#endif

        [[nodiscard]] const ShaderMap& GetShaders() const;
        [[nodiscard]] const TextureArray& GetTextures() const;
        [[nodiscard]] Weak<Shape> GetShape() const;

    protected:
        ShapeInstance();
        void Load_INTERNAL() override;
        void Unload_INTERNAL() override;
        
    private:
#if WITH_EDITOR
        bool m_b_ui_edit_resource_ = false;
        bool m_b_ui_add_resource_ = false;

        void ProcessEditUI();
        void ProcessAddUI();
#endif
        
        void addShaderImpl(const Strong<Shader>& shader);
        void addTextureImpl(const Strong<Texture>& texture);
        void addShapeImpl(const Strong<Shape>& shape);

        EPROPERTY()
        SBs::ShapeInstanceBindFlag m_instance_bind_flag_;
        
        EPROPERTY()
        std::unordered_set<MetadataPath> m_shader_paths_;

        EPROPERTY()
        MetadataPath m_atlas_path_;

        EPROPERTY()
        MetadataPath m_animations_path_;

        EPROPERTY()
        MetadataPath m_shape_path_;
        
        EPROPERTY()
        std::array<MetadataPath, BIND_SLOT_END> m_texture_paths_;
        
        ShaderMap m_shaders_loaded_;
        TextureArray m_textures_loaded_;
        Strong<AtlasAnimationTexture> m_atlas_loaded_;
        Strong<AnimationTexture> m_animations_loaded_;
        Strong<Shape> m_shape_loaded_;
    };
}
