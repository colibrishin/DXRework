#include "ShapeInstance.h"

#include "AnimationTexture.h"
#include "ShapeInstance.generated.h"
#include "UIHelpersResourceManager.h"

namespace Engine::Resources
{
    ShapeInstance::ShapeInstance(const std::vector<Weak<Resource>>& resources)
        : Resource("")
    {
        for (auto& resource : resources)
        {
            AddResource(resource);
        }
    }

    bool ShapeInstance::IsRenderDomain(const eShaderDomain domain) const noexcept
    {
        return m_shaders_loaded_.contains(domain);
    }

#if WITH_EDITOR
    void ShapeInstance::OnUIUpdate(UIContext* const parent, const float dt)
    {
        if (parent)
        {
            Resource::OnUIUpdate(parent, dt);
            UIInterface& ui = UIInterfaceAccessor::GetInterface();

            (*parent |= ui.NewButton({"Edit Resources"})).SetFunction([&]()
            {
                m_b_ui_edit_resource_ = !m_b_ui_edit_resource_;
            });

            ProcessEditUI();

            (*parent |= ui.NewButton({"Add Resources"})).SetFunction([&]()
            {
                m_b_ui_add_resource_ = !m_b_ui_add_resource_;
            });

            ProcessAddUI();
        }
    }

    void ShapeInstance::ProcessEditUI()
    {
        if (m_b_ui_edit_resource_)
        {
            UIInterface& ui = UIInterfaceAccessor::GetInterface();
            if (UIContext context = UIInterface::NewContext(ui.NewDialog({this, "Edit Resources", m_b_ui_edit_resource_})))
            {
                context += ui.NewListBox({"Resource Used", 0, 0});
                context >> ui.NewDragAndDropTarget({"RESOURCE", [&](void* ptr)
                {
                    if (auto casted = static_cast<Strong<Resource>*>(ptr))
                    {
                        AddResource(*casted);
                    }
                }});

                //todo: shape, texture, atlas, animation, shader in list

                for (auto& resources : m_resources_loaded_ | std::views::values)
                {
                    if (resources.empty())
                    {
                        continue;
                    }

                    const std::string_view type_name = (*resources.begin())->GetPrettyTypeName();
                    context += ui.NewTreeNode({type_name});

                    for (auto it = resources.begin(); it != resources.end(); ++it)
                    {
                        bool temp = false;
                        GlobalEntityID target_id = (*it)->GetID();

                        (context |= ui.NewSelectable({(*it)->GetName(), temp})).SetFunction([&, target_id]()
                        {
                            std::erase_if(resources, [target_id](const Strong<Resource>& value)
                            {
                                return value->GetID() == target_id;
                            });
                        });
                    }

                    --context;
                }

                --context;
            }
        }
    }

    void ShapeInstance::ProcessAddUI()
    {
        if (m_b_ui_add_resource_)
        {
            if (std::vector<Weak<Resource>> resources_to_load{};
                UIHelpers::MultipleResourceSelectionDialogInclusion<ShapeInstance, Texture, Shape, AtlasAnimationTexture, AnimationTexture, Shader>(
                    GetSharedPtr<ShapeInstance>(), resources_to_load))
            {
                m_b_ui_add_resource_ = false;

                for (const Weak<Resource>& resource : resources_to_load)
                {
                    if (const Strong<Resource>& locked = resource.lock()) 
                    {
                        AddResource(locked);
                    }
					
                }
            }
        }
    }
#endif

    const ShapeInstance::ShaderMap& ShapeInstance::GetShaders() const
    {
        return m_shaders_loaded_;
    }

    const ShapeInstance::TextureArray& ShapeInstance::GetTextures() const
    {
        return m_textures_loaded_;
    }

    Weak<Shape> ShapeInstance::GetShape() const
    {
        return m_shape_loaded_;
    }

    ShapeInstance::ShapeInstance()
        : Resource("") {}

    void ShapeInstance::Load_INTERNAL()
    {
        for (const auto& path : m_shader_paths_)
        {
            const Weak<Shader>& shader = Shader::GetByMetadataPath(path);
            
            if (const Strong<Shader> locked = shader.lock())
            {
                m_shaders_loaded_[locked->GetDomain()] = locked;
            }
        }
    }

    void ShapeInstance::Unload_INTERNAL()
    {
        m_shaders_loaded_.clear();
        m_textures_loaded_ = {};
        m_atlas_loaded_ = {};
        m_animations_loaded_ = {};
        m_shape_loaded_ = {};
    }

    void ShapeInstance::addShaderImpl(const Strong<Shader>& shader)
    {
        if (!shader->GetMetadataPath().empty() &&
                std::ranges::find_if(m_shader_paths_, [&shader](const MetadataPath& path)
                 {
                     return path == shader->GetMetadataPath();
                 }
                ) != m_shader_paths_.end())
        {
            return;
        }

        m_shader_paths_.insert(shader->GetMetadataPath());
        m_shaders_loaded_[shader->GetSharedPtr<Shader>()->GetDomain()] = shader;
    }

    void ShapeInstance::addTextureImpl(const Strong<Texture>& texture)
    {
        if (AtlasAnimationTexture::StaticIsBaseOf(texture->GetTypeHash()))
        {
            m_instance_bind_flag_.atlas = 1;
            m_atlas_loaded_ = texture->GetSharedPtr<AtlasAnimationTexture>();
            m_atlas_path_ = texture->GetMetadataPath();
        }
        else if (AnimationTexture::StaticIsBaseOf(texture->GetTypeHash()))
        {
            m_instance_bind_flag_.bone = 1;
            m_animations_loaded_ = texture->GetSharedPtr<AnimationTexture>();
            m_animations_path_ = texture->GetMetadataPath();
        }
        else
        {
            auto it = m_textures_loaded_.begin();
            for (; it != m_textures_loaded_.end(); ++it)
            {
                if (!*it)
                {
                    *it = texture;
                }
            }
            
            m_instance_bind_flag_.tex[std::distance(m_textures_loaded_.begin(), it)] = 1;
            m_texture_paths_[std::distance(m_textures_loaded_.begin(), it)] = texture->GetMetadataPath();
        }
    }

    void ShapeInstance::addShapeImpl(const Strong<Shape>& shape)
    {
        m_shape_loaded_ = shape;
        m_shape_path_ = shape->GetMetadataPath();
    }
}
