#include "ResourceManager.h"
#include "ResourceManager.generated.h"

#include <ranges>
#include "Resource.h"

#if WITH_EDITOR
#include "UIHelpersSceneManager.h"
#include "ObjectBase.h"
#include "Prefab.h"
#endif

namespace Engine::Managers
{
	void ResourceManager::Initialize() 
	{
#if WITH_EDITOR
		RegisterNewResource(Resources::Prefab::StaticTypeName(), [this](bool& managing_flag)
			{
				if (std::vector<Weak<Abstracts::ObjectBase>> object_selected;
					UIHelpers::MultipleObjectSelectionDialog<ResourceManager>(
						GetSharedPtr<ResourceManager>(), object_selected, {}, {}))
				{
					for (const auto& ptr : object_selected)
					{
						if (const Strong<Abstracts::ObjectBase>& object = ptr.lock())
						{
							Resources::Prefab::Create(object->GetName() + "_Prefab", object);
						}
					}

					managing_flag = false;
				}
			});
#endif
	}

#if WITH_EDITOR
	void ResourceManager::OnUIUpdate(UIContext* const parent, const float dt)
	{
		IUIAPI& ui = g_ui_accessor.GetInterface();

        if ( UIContext context = IUIAPI::NewContext(
                ui.NewDialog( this, "ResourceManagerDialog", { m_ui_info_.label, m_ui_info_.dialogOpened } ) ) )
        {
            for ( const auto &set : m_resources_ | std::views::values )
            {
                if ( set.empty() )
                {
                    continue;
                }

                const std::string_view type_name = ( *set.begin() )->GetPrettyTypeName();
                context += ui.NewTreeNode( this, "ResourceTreeNode", { type_name } );

                for ( auto it = set.begin(); it != set.end(); ++it )
                {
                    const Strong<Abstracts::Resource> &resource = *it;
                    const ptrdiff_t                    idx      = std::distance( set.begin(), it );

                    context += ui.NewSelectable( this,
                                                 std::format( "Resource{}", idx ),
                                                 { resource->GetName(), resource->m_ui_info_.dialogOpened } );
                    context >> ui.NewDragAndDropSource( this,
                                                        std::format( "ResourceDragAndDropSource{}", idx ),
                                                        { "RESOURCE",
                                                          resource->GetName(),
                                                          &resource,
                                                          sizeof( decltype(resource) ) } );

                    if ( resource->m_ui_info_.dialogOpened )
                    {
                        if ( UIContext resource_context = IUIAPI::NewContext(
                                ui.NewDialog( this,
                                              std::format( "ResourceDialog{}", idx ),
                                              { resource->m_ui_info_.label, resource->m_ui_info_.dialogOpened } ) ) )
                        {
                            resource->OnUIUpdate( &resource_context, dt );
                        }
                    }

                    --context;
                }

                --context;
            }
		}

        if ( UIContext menu_context = IUIAPI::NewContext( ui.NewMainMenuBar( nullptr, "MainMenuBar", {} ) ) )
        {
            menu_context += ui.NewMenu( nullptr, "NewMenu", { "New" } );

            for ( const auto &name : m_ui_new_functions_ | std::views::keys )
            {
                ( menu_context |= ui.NewMenuItem( this, std::format( "NewMenuItem{}", name ), { name } ) ).SetFunction(
                        [&]()
                        {
                            m_ui_new_functions_[ name ].first = true;
                        } );
            }

            --menu_context;

            menu_context += ui.NewMenu( nullptr, "LoadMenu", { "Load" } );

            for ( const auto &name : m_ui_load_functions_ | std::views::keys )
            {
                ( menu_context |= ui.NewMenuItem( this, std::format( "LoadMenuItem{}", name ), { name } ) ).SetFunction(
                        [&]()
                        {
                            m_ui_load_functions_[ name ].first = true;
                        } );
            }

            --menu_context;
        }

		for (auto& [flag, func] : m_ui_new_functions_ | std::views::values)
		{
			if (flag)
			{
				func(flag);
			}
		}

		for (auto& [flag, func] : m_ui_load_functions_ | std::views::values)
		{
			if (flag)
			{
				func(flag);
			}
		}
	}
#endif

	void ResourceManager::PreUpdate(const float dt)
	{
		for (const auto& resources : m_resources_ | std::views::values)
		{
			for (const Strong<Abstracts::Resource>& res : resources)
			{
				if (res.use_count() == 1 && res->IsLoaded())
				{
					res->Unload();
				}
			}
		}
	}

	void ResourceManager::Update(const float dt) {}

	void ResourceManager::PreRender(const float dt) {}

	void ResourceManager::PostUpdate(const float dt) {}

	void ResourceManager::Render(const float dt) {}

	void ResourceManager::PostRender(const float dt) {}

	void ResourceManager::FixedUpdate(const float dt) {}

	void ResourceManager::AddResource(const std::string_view name, const Strong<Abstracts::Resource>& resource, const ResourceType type)
	{
		AddResource(resource, type);
		resource->SetName(name);
	}

	void ResourceManager::AddResource(const Strong<Abstracts::Resource>& resource, const ResourceType type)
	{
		if (!resource->GetMetadataPath().empty() &&
			SearchResourceByMetadata(resource->GetMetadataPath(), type).lock())
		{
			return;
		}

		m_resources_[type].insert(resource);
	}

	inline Weak<Abstracts::Resource> ResourceManager::GetResource(const std::string_view name, ResourceType type)
	{
		auto& resources = m_resources_[type];
		const auto it = std::ranges::find_if
		(
			resources,
			[&name](const Strong<Abstracts::Resource>& resource)
			{
				return resource->GetName() == name;
			}
		);

		if (it != resources.end())
		{
			if (!(*it)->IsLoaded())
			{
				(*it)->Load();
			}

			return *it;
		}

		return {};
	}

	Weak<Abstracts::Resource> ResourceManager::GetResourceByMetadataPath(
		const std::filesystem::path& path, const ResourceType type
	)
	{
		if (const Strong<Abstracts::Resource>& resource = SearchResourceByMetadata(path, type).lock())
		{
			if (!resource->IsLoaded())
			{
				resource->Load();
			}

			return resource;
		}

		if (exists(path))
		{
			if (Strong<Abstracts::Resource> deserialized;
				Serializer::Deserialize<Abstracts::Resource>(path.generic_string(), deserialized))
			{
				if (!type->IsBaseOf(deserialized->GetTypeHash()))
				{
					return {};
				}

				AddResource(deserialized, deserialized->GetTypeHash());
				deserialized->Load();
				return deserialized;
			}
		}

		return {};
	}

#if WITH_EDITOR
	void ResourceManager::RegisterLoadResource(const std::string_view name, const UIHelpers::ManagedBooleanSignature& functor)
	{
		if (!m_ui_load_functions_.contains(name))
		{
			m_ui_load_functions_[name] = {false, functor};
		}
	}

	void ResourceManager::UnregisterLoadResource(const std::string_view name)
	{
		if (m_ui_load_functions_.contains(name))
		{
			m_ui_load_functions_.erase(name);
		}
	}

	void ResourceManager::RegisterNewResource(const std::string_view name, const UIHelpers::ManagedBooleanSignature& functor)
	{
		if (!m_ui_new_functions_.contains(name))
		{
			m_ui_new_functions_[name] = {false, functor};
		}
	}

	void ResourceManager::UnregisterNewResource(const std::string_view name)
	{
		if (m_ui_new_functions_.contains(name))
		{
			m_ui_new_functions_.erase(name);
		}
	}
#endif

	void ResourceManager::PreDeconstruction()
    {
        m_resource_cache_.clear();
        m_resource_ids_.clear();

        for ( auto& set : m_resources_ | std::views::values )
        {
            for ( Engine::Strong<Abstracts::Resource> resource : set )
            {
                resource->Unload();
                resource.reset();
            }
        }
	}

    ResourceManager::~ResourceManager()
	{
	}

	Weak<Abstracts::Resource> ResourceManager::SearchResourceByMetadata(
		const std::filesystem::path& path, ResourceType type
	) const
	{
		if (path.empty())
		{
			return {};
		}
		
		for (const auto& [res_type, resources] : m_resources_)
		{
			if (type == res_type || type->IsBaseOf(res_type))
			{
				const auto& it = std::ranges::find_if
				(
					resources, [&path](const Strong<Abstracts::Resource>& resource)
					{
						return resource->GetMetadataPath() == path;
					}
				);

				if (it != m_resources_.at(res_type).end())
				{
					return *it;
				}
			}
		}

		return {};
	}

#if WITH_EDITOR
	const ResourceManager::ResourceMap& ResourceManager::GetResources() const
	{
		return m_resources_;
	}
#endif
}
