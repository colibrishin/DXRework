#include "MaterialModule.h"
#include "Material.h"
#include "ModuleRegistration.h"
#include "ResourceManager.h"
#include "SceneManager.h"

MODULE_IMPL(Engine::MaterialModule, Material)

bool Engine::MaterialModule::InitializeImpl()
{
#if WITH_EDITOR
	Managers::ResourceManager::GetInstance().RegisterNewResource(Resources::Material::StaticTypeName(), [](bool& managing_flag)
		{
			Resources::Material::Create("NewMaterial", Graphics::MaterialPrimitive{});
			managing_flag = false;
		} ENGINE_MODULE_SCOPE );

	Managers::ResourceManager::GetInstance().RegisterLoadResource(Resources::Material::StaticTypeName(), [](bool& managing_flag)
		{
			const auto& load_callback = [](const std::string_view name, const std::string_view path)
				{
					Managers::ResourceManager::GetInstance().GetResourceByMetadataPath<Resources::Material>(path);
				};

			UIHelpers::OpenLoadDialog<Resources::Material, Managers::ResourceManager>(managing_flag, {}, load_callback, {});
		} ENGINE_MODULE_SCOPE );
#endif 

	return true;
}

bool Engine::MaterialModule::ShutdownImpl()
{
#if WITH_EDITOR
	Managers::ResourceManager::GetInstance().UnregisterNewResource(Resources::Material::StaticTypeName());
	Managers::ResourceManager::GetInstance().UnregisterLoadResource(Resources::Material::StaticTypeName());
#endif

	return true;
}

bool Engine::MaterialModule::DynamicLoadable()
{
	return true;
}
