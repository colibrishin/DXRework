#pragma once
#include <ranges>
#include "Source/Runtime/Abstracts/CoreSingleton/Public/Singleton.hpp"
#include <Source/Runtime/TypeLibrary/Public/TypeLibrary.h>
#include <Source/Runtime/Allocator/Public/Allocator.h>

namespace Engine::Managers
{
	class ResourceManager : public Engine::Abstracts::Singleton<ResourceManager>
	{
	public:
		explicit ResourceManager(SINGLETON_LOCK_TOKEN) {}

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void PostUpdate(const float& dt) override;
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;
		void FixedUpdate(const float& dt) override;

		template <typename T, typename ResLock = std::enable_if_t<std::is_base_of_v<Abstracts::Resource, T>>>
		void AddResource(const boost::shared_ptr<T>& resource)
		{
			if (!resource->GetMetadataPath().empty() &&
			    GetResourceByMetadataPath<T>(resource->GetMetadataPath()).lock())
			{
				return;
			}
			if (!resource->GetPath().empty() &&
			    GetResourceByRawPath<T>(resource->GetPath()).lock())
			{
				return;
			}

			m_resources_[which_resource<T>::value].insert(resource);
		}

		template <typename T, typename ResLock = std::enable_if_t<std::is_base_of_v<Abstracts::Resource, T>>>
		void AddResource(const EntityName& name, const boost::shared_ptr<T>& resource)
		{
			if (!resource->GetMetadataPath().empty() &&
			    GetResourceByMetadataPath<T>(resource->GetMetadataPath()).lock())
			{
				return;
			}
			if (!resource->GetPath().empty() &&
			    GetResourceByRawPath<T>(resource->GetPath()).lock())
			{
				return;
			}

			m_resources_[which_resource<T>::value].insert(resource);
			resource->SetName(name);
		}

		template <typename T>
		boost::weak_ptr<T> GetResource(const EntityName& name)
		{
			auto& resources = m_resources_[which_resource<T>::value];
			auto  it        = std::find_if
					(
					 resources.begin(), resources.end(), [&name](const Strong<Abstracts::Resource>& resource)
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

				return boost::static_pointer_cast<T>(*it);
			}

			return {};
		}

		Weak<Abstracts::Resource> GetResource(const EntityName& name, const eResourceType& type);

		template <typename T>
		boost::weak_ptr<T> GetResourceByRawPath(const std::filesystem::path& path)
		{
			if (path.empty())
			{
				return {};
			}

			auto& resources = m_resources_[which_resource<T>::value];
			auto  it        = std::find_if
					(
					 resources.begin(), resources.end(), [&path](const Strong<Abstracts::Resource>& resource)
					 {
						 return resource->GetPath() == path;
					 }
					);

			if (it != resources.end())
			{
				if (!(*it)->IsLoaded())
				{
					(*it)->Load();
				}

				return boost::reinterpret_pointer_cast<T>(*it);
			}

			return {};
		}

		template <typename T>
		boost::weak_ptr<T> GetResourceByMetadataPath(const std::filesystem::path& path)
		{
			if (path.empty())
			{
				return {};
			}

			auto& resources = m_resources_[which_resource<T>::value];
			auto  it        = std::find_if
					(
					 resources.begin(), resources.end(), [&path](const Strong<Abstracts::Resource>& resource)
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

				return boost::reinterpret_pointer_cast<T>(*it);
			}
			if (exists(path))
			{
				const auto res = Serializer::Deserialize<Entity>(path.generic_string())->GetSharedPtr<T>();
				m_resources_[which_resource<T>::value].insert(res);
				res->Load();
				return res;
			}

			return {};
		}

		Weak<Abstracts::Resource> GetResourceByRawPath(const std::filesystem::path& path, eResourceType type);
		Weak<Abstracts::Resource> GetResourceByMetadataPath(const std::filesystem::path& path, eResourceType type);

	private:
		friend struct SingletonDeleter;
		~ResourceManager() override = default;

		fast_pool_unordered_map<eResourceType, fast_pool_set<Strong<Abstracts::Resource>>> m_resources_;
		fast_pool_unordered_map<LocalResourceID, Weak<Abstracts::Resource>> m_resource_cache_;
		fast_pool_unordered_map<LocalResourceID, GlobalEntityID> m_resource_ids_;
	};
} // namespace Engine::Manager
