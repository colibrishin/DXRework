#include "AtlasAnimationTextureModule.h"
#include "AtlasAnimationTextureModule.generated.h"

#include "AtlasAnimationTexture.h"
#include "AtlasAnimation.h"
#include "ModuleManager/Public/ModuleManager.h"
#include "ResourceManager/Public/ResourceManager.h"

#include <string.h>

MODULE_IMPL(Engine::AtlasAnimationTextureModule, AtlasAnimationTexture)

void Engine::AtlasAnimationTextureModule::Initialize()
{
    Managers::ResourceManager::GetInstance().RegisterLoadResource(Engine::Resources::AtlasAnimationTexture::StaticTypeName(), [](bool& managing_flag)
        {
            const auto& load_callback = [](const std::string_view name, const std::string_view path) 
                {
                    Managers::ResourceManager::GetInstance().GetResourceByMetadataPath<Engine::Resources::AtlasAnimationTexture>(path);
                };
            
            UIHelpers::OpenLoadDialog<Resources::AtlasAnimationTexture, Managers::ResourceManager>(
                managing_flag,
                {},
                load_callback,
                {});
        });

    Managers::ResourceManager::GetInstance().RegisterNewResource(Engine::Resources::AtlasAnimationTexture::StaticTypeName(), [](bool& managing_flag)
        {
            static std::string sub_atlas_name_buffer;
            static std::string sub_atlas_texture_path_buffer;
            static std::string sub_atlas_xml_path_buffer;
            static std::vector<std::tuple< std::string, std::string, std::string>> listed_pair;

            static constexpr auto clearInputBuffers = []()
                {
                    std::memset(sub_atlas_name_buffer.data(), 0, sizeof(decltype(sub_atlas_name_buffer)::value_type) * sub_atlas_name_buffer.size());
                    std::memset(sub_atlas_texture_path_buffer.data(), 0, sizeof(decltype(sub_atlas_texture_path_buffer)::value_type) * sub_atlas_texture_path_buffer.size());
                    std::memset(sub_atlas_xml_path_buffer.data(), 0, sizeof(decltype(sub_atlas_xml_path_buffer)::value_type) * sub_atlas_xml_path_buffer.size());
                };

            const auto& ui_callback = [](UIContext* const context)
                {
                    UIInterface& ui = UIInterfaceAccessor::GetInterface();
                    constexpr const char* expected_tex_extensions[] = { ".png", ".jpg", ".bmp" ".dds" };

                    *context |= ui.NewLabelAndText({ "Atlas Name", sub_atlas_name_buffer, true });
                    *context |= ui.NewLabelAndText({ "Atlas Texture", sub_atlas_texture_path_buffer, true });
                    *context |= ui.NewLabelAndText({ "Atlas XML", sub_atlas_xml_path_buffer, true });
                    (*context |= ui.NewButton({ "Add" })).SetFunction([&expected_tex_extensions]()
                        {
                            if (!sub_atlas_name_buffer.empty() && sub_atlas_xml_path_buffer.empty()) 
                            {
                                const std::filesystem::path tex_path = sub_atlas_texture_path_buffer;
                                std::filesystem::path       expected_xml = tex_path.filename();
                                expected_xml.replace_extension(".xml");

                                const std::filesystem::path               folder = tex_path.parent_path();
                                const std::filesystem::directory_iterator it(folder);

                                for (const auto& entry : it)
                                {
                                    if (const std::filesystem::path& p = entry.path();
                                        p.filename() == expected_xml)
                                    {
                                        if (sub_atlas_xml_path_buffer.size() < p.generic_string().size()) 
                                        {
                                            sub_atlas_xml_path_buffer.resize(p.generic_string().size());
                                        }

                                        strncpy_s(
                                            sub_atlas_xml_path_buffer.data(),
                                            sizeof(decltype(sub_atlas_xml_path_buffer)::value_type),
                                            p.generic_string().c_str(),
                                            p.generic_string().size());
                                        break;
                                    }
                                }
                            }

                            if (sub_atlas_texture_path_buffer.empty() && !sub_atlas_xml_path_buffer.empty())
                            {
                                const std::filesystem::path xml_path = sub_atlas_xml_path_buffer;
                                std::filesystem::path       expected_tex = xml_path.stem();

                                const std::filesystem::path               folder = xml_path.parent_path();
                                const std::filesystem::directory_iterator it(folder);

                                for (const auto& entry : it)
                                {
                                    if (const std::filesystem::path& p = entry.path();
                                        p.stem() == expected_tex && 
                                        std::find(std::begin(expected_tex_extensions), std::end(expected_tex_extensions), p.extension()) != std::end(expected_tex_extensions))
                                    {
                                        strncpy_s(
                                            sub_atlas_texture_path_buffer.data(),
                                            sizeof(decltype(sub_atlas_texture_path_buffer)::value_type),
                                            p.generic_string().c_str(),
                                            p.generic_string().size());
                                        break;
                                    }
                                }
                            }

                            if (std::filesystem::exists(sub_atlas_texture_path_buffer) && std::filesystem::exists(sub_atlas_xml_path_buffer))
                            {
                                // create a copy of buffers
                                listed_pair.push_back({ sub_atlas_name_buffer, sub_atlas_texture_path_buffer, sub_atlas_xml_path_buffer });
                                clearInputBuffers();
                            }
                        });

                    static std::string search_folder;
                    *context |= ui.NewLabelAndText({ "Folder", search_folder, true });
                    (*context |= ui.NewButton({ "Add Multiples..." })).SetFunction([&expected_tex_extensions]()
                        {
                            std::filesystem::path                               folder = search_folder;
                            const std::filesystem::recursive_directory_iterator it(folder);

                            for (const auto& entry : it)
                            {
                                if (const std::filesystem::path& xml_entry = entry.path();
                                    xml_entry.extension() == ".xml")
                                {
                                    std::any_of(std::begin(expected_tex_extensions), std::end(expected_tex_extensions), [xml_entry](const char* extension)
                                        {
                                            std::filesystem::path tex_path = xml_entry;
                                            tex_path.replace_extension(extension);

                                            if (std::filesystem::exists(tex_path))
                                            {
                                                listed_pair.emplace_back
                                                (
                                                    xml_entry.stem().generic_string(),
                                                    tex_path.generic_string(),
                                                    xml_entry.generic_string()
                                                );

                                                return true;
                                            }

                                            return false;
                                        });
                                }
                            }
                        });

                    *context += ui.NewListBox({ "ListedAtlasTexture", -1, 300 });
                    for (const auto& [name, tex_path, xml_path] : listed_pair) 
                    {
                        *context |= ui.NewText({ name });
                        *context |= ui.NewText({ tex_path });
                        *context |= ui.NewText({ xml_path });
                        *context |= ui.NewSeparator({});
                    }
                    --*context;
                };

            const auto& load_callback = [](const std::string_view name, const std::string_view path)
                {
                    try
                    {
                        std::vector<Strong<Resources::AtlasAnimation>> animations;
                        std::vector<Strong<Resources::Texture2D>> textures;

                        for (const auto& [sub_atlas_name, tex_path, xml_path] : listed_pair)
                        {
                            animations.emplace_back(Resources::AtlasAnimation::Create(sub_atlas_name, xml_path));
                            textures.emplace_back(Resources::Texture2D::Create(sub_atlas_name, tex_path, GenericTextureDescription{}));
                        }

                        Resources::AtlasAnimationTexture::Create(name.data(), "", animations, textures);
                    }
                    catch (std::exception e)
                    {
                        clearInputBuffers();
                    }
                };

            const auto& cleanup_callback = []()
                {
                    clearInputBuffers();
                };

            UIHelpers::OpenNewDialog<Resources::AtlasAnimationTexture, Managers::ResourceManager>(
                managing_flag, 
                ui_callback, 
                load_callback,
                cleanup_callback);
        });
}

void Engine::AtlasAnimationTextureModule::Shutdown()
{
    Managers::ResourceManager::GetInstance().UnregisterNewResource(Engine::Resources::AtlasAnimationTexture::StaticTypeName());
    Managers::ResourceManager::GetInstance().UnregisterLoadResource(Engine::Resources::AtlasAnimationTexture::StaticTypeName());
}

bool Engine::AtlasAnimationTextureModule::DynamicLoadable()
{
    return true;
}
