#include "ShapeModule.h"
#include "Shape.h"

#include "ModuleManager/Public/ModuleManager.h"
#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.h"
#include "ShapeModule.generated.h"

MODULE_IMPL(Engine::ShapeModule, Shape)

void Engine::ShapeModule::Initialize()
{
	Managers::ResourceManager::GetInstance().RegisterNewResource(Resources::Shape::StaticTypeName(), [](bool& managing_flag)
		{
			const auto& load_callback = [](const std::string_view name, const std::string_view path)
				{
					Resources::Shape::Create(name.data(), path);
				};

			// todo: coordination system
			Managers::ResourceManager::GetInstance().OpenNewDialog<Resources::Shape>(managing_flag, {}, load_callback, {});
		});

	Managers::ResourceManager::GetInstance().RegisterLoadResource(Resources::Shape::StaticTypeName(), [](bool& managing_flag)
	{
		UIInterface& ui = UIInterfaceAccessor::GetInterface();

		const auto& load_callback = [](const std::string& name, const std::string& path)
		{
			try
			{
				if (path.empty())
				{
					return;
				}
				else
				{
					if (std::filesystem::exists(path))
					{
						Managers::ResourceManager::GetInstance().GetResourceByMetadataPath<Engine::Resources::Shape>(path);
					}
				}
			}
			catch (std::exception e)
			{
				return;
			}
		};

		Managers::ResourceManager::GetInstance().OpenLoadDialog<Resources::Shape>(managing_flag, {}, load_callback, {});
	});
}

void Engine::ShapeModule::Shutdown()
{
	Managers::ResourceManager::GetInstance().UnregisterNewResource(Resources::Shape::StaticTypeName());
	Managers::ResourceManager::GetInstance().UnregisterLoadResource(Resources::Shape::StaticTypeName());	
}

bool Engine::ShapeModule::DynamicLoadable()
{
	return true;
}
