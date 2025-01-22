#include "../Public/ResourceManager.hpp"

#include <ranges>
#include "Source/Runtime/Core/Resource/Public/Resource.h"

namespace Engine::Managers
{
	void ResourceManager::Initialize() {}

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

	inline Weak<Abstracts::Resource> ResourceManager::GetResource(const EntityName& name, ResourceType type)
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
