#include "SoundPlayerModule.h"
#include "SoundPlayerModule.generated.h"

#include "SoundPlayer.h"

MODULE_IMPL(Engine::SoundPlayerModule, SoundPlayer)

bool Engine::SoundPlayerModule::InitializeImpl()
{
	Engine::ComponentFactory::Register<Components::SoundPlayer>();
	return true;
}

bool Engine::SoundPlayerModule::ShutdownImpl()
{
	Engine::ComponentFactory::Unregister<Components::SoundPlayer>();
	return true;
}

bool Engine::SoundPlayerModule::DynamicLoadable()
{
	return false;
}
