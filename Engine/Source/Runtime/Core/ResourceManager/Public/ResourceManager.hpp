#pragma once
#include <ranges>
#include "Source/Runtime/Core/Resource/Public/Resource.h"
#include "Source/Runtime/CoreSingleton/Public/Singleton.hpp"
#include "Source/Runtime/Core/Allocator/Public/Allocator.h"

POLYMORPHIC_MANAGER_TYPE_MAP(Engine::Managers::ResourceManager)

namespace Engine::Managers
{
	class ENGINE_CORE_API ResourceManager : public Engine::Abstracts::Singleton<ResourceManager>
	{
	public:
		INLINE_COMPILE_TIME_TYPENAME(ResourceManager)
		explicit ResourceManager(SINGLETON_LOCK_TOKEN) {}

		void Initialize() override;

		void OnUIUpdate(UIContext* const parent, const float dt) override;
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void PreRender(const float dt) override;
		void PostUpdate(const float dt) override;
		void Render(const float dt) override;
		void PostRender(const float dt) override;
		void FixedUpdate(const float dt) override;

		template <typename T, typename ResLock = std::enable_if_t<std::is_base_of_v<Abstracts::Resource, T>>>
		void AddResource(const Strong<T>& resource)
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

			m_resources_[T::StaticTypeHash()].insert(resource);
		}

		template <typename T, typename ResLock = std::enable_if_t<std::is_base_of_v<Abstracts::Resource, T>>>
		void AddResource(const EntityName& name, const Strong<T>& resource)
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

			m_resources_[T::StaticTypeHash()].insert(resource);
			resource->SetName(name);
		}

		template <typename T>
		Weak<T> GetResource(const EntityName& name)
		{
			auto& resources = m_resources_[T::StaticTypeHash()];
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

		Weak<Abstracts::Resource> GetResource(const EntityName& name, ResourceType type);

		template <typename T>
		Weak<T> GetResourceByRawPath(const std::filesystem::path& path)
		{
			if (path.empty())
			{
				return {};
			}

			auto& resources = m_resources_[T::StaticTypeHash()];
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
		Weak<T> GetResourceByMetadataPath(const std::filesystem::path& path)
		{
			if (path.empty())
			{
				return {};
			}

			auto& resources = m_resources_[T::StaticTypeHash()];
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

			return {};
		}

		Weak<Abstracts::Resource> GetResourceByRawPath(const std::filesystem::path& path, ResourceType type);
		Weak<Abstracts::Resource> GetResourceByMetadataPath(const std::filesystem::path& path, ResourceType type);

#if WITH_EDITOR
		[[nodiscard]] bool RequestAddResourceDialog();
		void EndAddResourceDialog();
		[[nodiscard]] bool TryAddResourceDialog(std::vector<Strong<Abstracts::Resource>>& resource_to_load);

	private:
		bool m_b_ui_add_resource_ = false;
#endif
	private:
		friend struct SingletonDeleter;
		~ResourceManager() override;

		fast_pool_unordered_map<ResourceType, fast_pool_set<Strong<Abstracts::Resource>>> m_resources_;
		fast_pool_unordered_map<LocalResourceID, Weak<Abstracts::Resource>> m_resource_cache_;
		fast_pool_unordered_map<LocalResourceID, GlobalEntityID> m_resource_ids_;
	};
} // namespace Engine::Managers
