#pragma once
#include <ranges>
#include "Source/Runtime/Core/Resource/Public/Resource.h"
#include "Source/Runtime/CoreSingleton/Public/Singleton.h"
#include "Source/Runtime/Core/Allocator/Public/Allocator.h"

#include "Serialization.hpp"

#include "ResourceManager.generated.h"
#include "UIHelpers.h"

namespace Engine::Managers
{
	ECLASS()
	class ENGINE_CORE_API ResourceManager : public Engine::Abstracts::Singleton<ResourceManager>
	{
		GENERATE_BODY
	public:
		explicit ResourceManager(SINGLETON_LOCK_TOKEN) {}

		void Initialize() override;

#ifdef WITH_EDITOR
		void OnUIUpdate(UIContext* const parent, const float dt) override;
#endif
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
		void AddResource(const std::string_view name, const Strong<T>& resource)
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
		Weak<T> GetResource(const std::string_view name)
		{
			if (const Strong<Abstracts::Resource>& locked = GetResource(name, T::StaticTypeHash()).lock()) 
			{
				return boost::reinterpret_pointer_cast<T>(locked);
			}
			
			return {};
		}

		Weak<Abstracts::Resource> GetResource(const std::string_view name, ResourceType type);

		template <typename T>
		Weak<T> GetResourceByRawPath(const std::filesystem::path& path)
		{
			if (const Strong<Abstracts::Resource>& locked = GetResourceByRawPath(path, T::StaticTypeHash()).lock()) 
			{
				return boost::reinterpret_pointer_cast<T>(locked);
			}

			return {};
		}

		template <typename T>
		Weak<T> GetResourceByMetadataPath(const std::filesystem::path& path)
		{
			if (const Strong<Abstracts::Resource>& found = GetResourceByMetadataPath(path, T::StaticTypeHash()).lock()) 
			{
				return boost::reinterpret_pointer_cast<T>(found);
			}

			if (std::filesystem::exists(path)) 
			{
				Strong<T> deserialized = Serializer::Deserialize<T>(path.generic_string());
				AddResource(deserialized);
				deserialized->Load();
				return deserialized;
			}

			return {};
		}

		Weak<Abstracts::Resource> GetResourceByRawPath(const std::filesystem::path& path, ResourceType type);
		Weak<Abstracts::Resource> GetResourceByMetadataPath(const std::filesystem::path& path, ResourceType type);

#if WITH_EDITOR
		void RegisterLoadResource(const std::string_view name, const UIHelpers::ManagedBooleanSignature& functor);
		void UnregisterLoadResource(const std::string_view name);
		auto RegisterNewResource(const std::string_view name, const UIHelpers::ManagedBooleanSignature& functor) -> void;
		void UnregisterNewResource(const std::string_view name);

		[[nodiscard]] bool RequestAddResourceDialog();
		void EndAddResourceDialog();
		[[nodiscard]] bool TryAddResourceDialog(std::vector<Strong<Abstracts::Resource>>& resource_to_load);
		
	private:
		bool m_b_ui_add_resource_ = false;

		UIHelpers::ManagedBoolAndFuncMap<std::string_view> m_ui_load_functions_;
		UIHelpers::ManagedBoolAndFuncMap<std::string_view> m_ui_new_functions_;
#endif
	private:
		ResourceManager() = default;
		friend struct SingletonDeleter;
		~ResourceManager() override;

		using ResourceMap = fast_pool_unordered_map<ResourceType, fast_pool_set<Strong<Abstracts::Resource>>>;

		ResourceMap m_resources_;
		fast_pool_unordered_map<LocalResourceID, Weak<Abstracts::Resource>> m_resource_cache_;
		fast_pool_unordered_map<LocalResourceID, GlobalEntityID> m_resource_ids_;

#if WITH_EDITOR
	public:
		const ResourceMap& GetResources() const;
#endif
	};
} // namespace Engine::Managers
