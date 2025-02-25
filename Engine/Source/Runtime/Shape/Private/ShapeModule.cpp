#include "ShapeModule.h"
#include "Shape.h"


#include "ResourceManager.h"
#include "ShapeModule.generated.h"

MODULE_IMPL(Engine::ShapeModule, Shape)

bool Engine::ShapeModule::InitializeImpl()
{
#if WITH_EDITOR
	Managers::ResourceManager::GetInstance().RegisterNewResource(Resources::Shape::StaticTypeName(), [](bool& managing_flag)
		{
            const auto &ui_callback = []( UIContext *const context )
            {
                UIInterface &ui = UIInterfaceAccessor::GetInterface();
                *context |= ui.NewText( nullptr,
                                        "ShapeNoteText1",
                                        { "Please Note that the path should be the location of the mesh file (e.g., obj, fbx)" } );
                *context |= ui.NewText( nullptr,
                                        "ShapeNoteText2",
                                        { "Path can be leaved empty if shape will be constructed in runtime." } );
            };

            const auto &load_callback = []( const std::string_view name, const std::string_view path )
            {
                Resources::Shape::Create( name.data(), path );
            };

            // todo: coordination system
            UIHelpers::OpenNewDialog<Resources::Shape, Managers::ResourceManager>(
                    managing_flag,
                    ui_callback,
                    load_callback,
                    {} );
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
						Managers::ResourceManager::GetInstance().GetResourceByMetadataPath<Resources::Shape>(path);
					}
				}
			}
			catch (std::exception e)
			{
				return;
			}
		};

		UIHelpers::OpenLoadDialog<Resources::Shape, Managers::ResourceManager>(managing_flag, {}, load_callback, {});
	});
#endif

	return true;
}

bool Engine::ShapeModule::ShutdownImpl()
{
#if WITH_EDITOR
	Managers::ResourceManager::GetInstance().UnregisterNewResource(Resources::Shape::StaticTypeName());
	Managers::ResourceManager::GetInstance().UnregisterLoadResource(Resources::Shape::StaticTypeName());
#endif
	return true;
}

bool Engine::ShapeModule::DynamicLoadable()
{
	return true;
}
