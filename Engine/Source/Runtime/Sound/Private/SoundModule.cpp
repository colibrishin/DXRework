#include "SoundModule.h"
#include "SoundModule.generated.h"

#include "Sound.h"

#include "UIHelpersResourceManager.h"
#include "ResourceManager.h"


MODULE_IMPL(Engine::SoundModule, Sound)

bool Engine::SoundModule::InitializeImpl()
{
    Resources::Sound::Create("street", "street.mp3");
#if WITH_EDITOR
    Managers::ResourceManager::GetInstance().RegisterLoadResource(Resources::Sound::StaticTypeName(), [](bool& managed_flag)
        {
            const auto& load_callback = [](const std::string_view name, const std::string_view path)
                {
                    Resources::Sound::Create(name.data(), path);
                };

            UIHelpers::OpenLoadDialog<Resources::Sound, Managers::ResourceManager>(
                managed_flag,
                {},
                load_callback,
                {}
            );
        });
#endif
    return true;
}

bool Engine::SoundModule::ShutdownImpl()
{
#if WITH_EDITOR
    Managers::ResourceManager::GetInstance().UnregisterLoadResource(Resources::Sound::StaticTypeName());
#endif
    return true;
}

bool Engine::SoundModule::DynamicLoadable()
{
    return true;
}

const std::vector<std::string>& Engine::SoundModule::LoadAfter() const
{
    static const std::vector<std::string> load_after = {
        "SoundManager",
#if USE_FMOD
        "FMODSoundInterface"
#endif
    };
    return load_after;
}
