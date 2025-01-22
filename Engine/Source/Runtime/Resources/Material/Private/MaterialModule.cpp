#include "MaterialModule.h"
#include "Material.h"
#include "SceneManager/Public/SceneManager.h"
#include "ResourceManager/Public/ResourceManager.h"

MODULE_IMPL(Engine::MaterialModule, Material)

void Engine::MaterialModule::Initialize()
{
	Managers::SceneManager::GetInstance().RegisterNewMenuItem(Resources::Material::StaticTypeName(), []()
		{
			Resources::Material::Create("NewMaterial", "");
		});

	Managers::SceneManager::GetInstance().RegisterLoadMenuItem(Resources::Material::StaticTypeName(), [](std::string_view path)
		{
			Managers::ResourceManager::GetInstance().GetResourceByMetadataPath<Resources::Material>(path);
		});
}

void Engine::MaterialModule::Shutdown()
{
	Managers::SceneManager::GetInstance().UnregisterNewMenuItem(Resources::Material::StaticTypeName());
	Managers::SceneManager::GetInstance().UnregisterLoadMenuItem(Resources::Material::StaticTypeName());
}

bool Engine::MaterialModule::DynamicLoadable()
{
	return true;
}
