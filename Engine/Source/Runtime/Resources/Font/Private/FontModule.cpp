#include "FontModule.h"
#include "FontModule.generated.h"

#include "Font.h"



#if WITH_EDITOR
#include "UIHelpersResourceManager.h"
#endif

MODULE_IMPL(Engine::FontModule, Font)

bool Engine::FontModule::InitializeImpl()
{
    Resources::Font::Create("consolas", "consolas.spritefont");
#if WITH_EDITOR
    Managers::ResourceManager::GetInstance().RegisterLoadResource(Resources::Font::StaticTypeName(), [](bool& managed_flag)
        {
            const auto& load_callback = [](const std::string_view name, const std::string_view path)
                {
                    Resources::Font::Create(name.data(), path);
                };

            UIHelpers::OpenLoadDialog<Resources::Font, Managers::ResourceManager>(
                managed_flag,
                {},
                load_callback,
                {}
            );
        });
#endif
    return true;
}

bool Engine::FontModule::ShutdownImpl()
{
#if WITH_EDITOR
    Managers::ResourceManager::GetInstance().UnregisterLoadResource(Resources::Font::StaticTypeName());
#endif
    return true;
}

bool Engine::FontModule::DynamicLoadable()
{
    return true;
}
