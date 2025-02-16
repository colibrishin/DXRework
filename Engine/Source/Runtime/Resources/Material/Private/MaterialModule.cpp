#include "MaterialModule.h"
#include "MaterialModule.generated.h"
#include "Material.h"
#include "SceneManager/Public/SceneManager.h"
#include "ResourceManager/Public/ResourceManager.h"
#include "ModuleManager/Public/ModuleManager.h"

MODULE_IMPL(Engine::MaterialModule, Material)

bool Engine::MaterialModule::InitializeImpl()
{
	Managers::ResourceManager::GetInstance().RegisterNewResource(Resources::Material::StaticTypeName(), [](bool& managing_flag)
		{
			Resources::Material::Create("NewMaterial", Graphics::MaterialPrimitive{});
			managing_flag = false;
		});

	Managers::ResourceManager::GetInstance().RegisterLoadResource(Resources::Material::StaticTypeName(), [](bool& managing_flag)
		{
			const auto& load_callback = [](const std::string_view name, const std::string_view path)
				{
					Managers::ResourceManager::GetInstance().GetResourceByMetadataPath<Resources::Material>(path);
				};

			UIHelpers::OpenLoadDialog<Resources::Material, Managers::ResourceManager>(managing_flag, {}, load_callback, {});
			
		});

	return true;
}

bool Engine::MaterialModule::ShutdownImpl()
{
	Managers::ResourceManager::GetInstance().UnregisterNewResource(Resources::Material::StaticTypeName());
	Managers::ResourceManager::GetInstance().UnregisterLoadResource(Resources::Material::StaticTypeName());

	return true;
}

bool Engine::MaterialModule::DynamicLoadable()
{
	return true;
}
