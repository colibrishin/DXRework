#include "../Public/ResourceManager.h"
#include "ResourceManager.generated.h"

#include <ranges>
#include "Source/Runtime/Core/Resource/Public/Resource.h"

namespace Engine::Managers
{
	void ResourceManager::Initialize() {}

#ifdef WITH_EDITOR
	void ResourceManager::OnUIUpdate(UIContext* const parent, const float dt)
	{
		UIInterface& ui = UIInterfaceAccessor::GetInterface();

		if (UIContext context = UIInterface::NewContext(ui.NewDialog({this, "Resource Manager", m_ui_info_.dialogOpened})))
		{
			for (const auto& set : m_resources_ | std::views::values)
			{
				if (set.empty())
				{
					continue;
				}

				const std::string_view type_name = (*set.begin())->GetPrettyTypeName();
				context += ui.NewTreeNode({type_name});

				for (const Strong<Abstracts::Resource>& resource : set)
				{
					context += ui.NewSelectable({resource->GetName(), resource->m_ui_info_.dialogOpened});
					context >> ui.NewDragAndDropSource({"RESOURCE", resource->GetName(), &resource, sizeof(decltype(resource))});

					if (resource->m_ui_info_.dialogOpened)
					{
						if (UIContext resource_context = UIInterface::NewContext(ui.NewDialog({resource.get(), resource->GetName(), resource->m_ui_info_.dialogOpened})))
						{
							resource->OnUIUpdate(&resource_context, dt);
						}
					}

					--context;
				}

				--context;
			}
		}

		if (UIContext menu_context = UIInterface::NewContext(ui.NewMainMenuBar({})))
		{
			menu_context += ui.NewMenu({ "New" });

			for (const auto& name : m_ui_new_functions_ | std::views::keys) 
			{
				(menu_context |= ui.NewMenuItem({ name })).SetFunction([&]()
					{
						m_ui_new_functions_[name].first = true;
					});
			}

			--menu_context;

			menu_context += ui.NewMenu({"Load"});

			for (const auto& name : m_ui_load_functions_ | std::views::keys)
			{
				(menu_context |= ui.NewMenuItem({name})).SetFunction([&]()
					{
						m_ui_load_functions_[name].first = true;
					});
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
			for (const auto& res : resources)
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

	Weak<Abstracts::Resource> ResourceManager::GetResourceByRawPath(const std::filesystem::path& path, const ResourceType type)
	{
		if (path.empty())
		{
			return {};
		}

		auto& resources = m_resources_[type];
		auto  it        = std::ranges::find_if(
				 resources, [&path](const Strong<Abstracts::Resource>& resource)
				 {
					 return resource->GetPath() == path;
				 });

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
		if (path.empty())
		{
			return {};
		}

		auto& resources = m_resources_[type];
		auto  it        = std::ranges::find_if
				(
				 resources, [&path](const Strong<Abstracts::Resource>& resource)
				 {
					 return resource->GetMetadataPath() == path;
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

	bool ResourceManager::RequestAddResourceDialog()
	{
		if (m_b_ui_add_resource_)
		{
			return false;
		}

		m_b_ui_add_resource_ = true;
		return true;
	}

	void ResourceManager::EndAddResourceDialog()
	{
		if (m_b_ui_add_resource_)
		{
			m_b_ui_add_resource_ = false;
		}
	}

	bool ResourceManager::TryAddResourceDialog(std::vector<Strong<Abstracts::Resource>>& resource_to_load)
	{
		bool                                                       window = true;
		static std::unordered_map<Weak<Abstracts::Resource>, bool> selection{};

		UIInterface& ui = UIInterfaceAccessor::GetInterface();
		if (UIContext context = UIInterface::NewContext(ui.NewDialog({this, "Add Resources to...", window})))
		{
			context += ui.NewListBox({"Resource List", -1, -1});

			for (const auto& resources : m_resources_ | std::views::values)
			{
				if (resources.empty())
				{
					continue;
				}

				const std::string_view type_name = (*resources.begin())->GetPrettyTypeName();

				context += ui.NewTreeNode({type_name});

				for (const Strong<Abstracts::Resource>& resource : resources)
				{
					context |= ui.NewSelectable({resource->GetName(), selection[resource]});
				}

				--context;
			}

			--context;

			(context |= ui.NewButton({"Add Resources"})).SetFunction([&window]()
			{
				window = false;
			});
		}

		if (!window)
		{
			resource_to_load.reserve(selection.size());

			for (const auto& key : selection | std::views::keys)
			{
				if (const Strong<Abstracts::Resource>& resource = key.lock())
				{
					if (!resource->IsLoaded())
					{
						resource->Load();
					}

					resource_to_load.push_back(resource);
				}
			}

			selection.clear();
			m_b_ui_add_resource_ = false;
		}

		return !window;
	}
#endif

	ResourceManager::~ResourceManager()
	{
		m_resource_cache_.clear();
		m_resource_ids_.clear();

		for (auto& set : m_resources_ | std::views::values)
		{
			for (boost::shared_ptr<Abstracts::Resource> resource : set)
			{
				resource->Unload();
				resource.reset();
			}
		}
	}
}
