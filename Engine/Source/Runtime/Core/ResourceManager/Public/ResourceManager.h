#pragma once
#include <ranges>
#include "Source/Runtime/Core/Resource/Public/Resource.h"
#include "Singleton.h"
#include "Source/Runtime/Core/Allocator/Public/Allocator.h"
#include "Serialization.hpp"

#if WITH_EDITOR
#include "UIHelpers.h"
#endif

#include "ResourceManager.generated.h"

namespace Engine::Managers
{
	ECLASS()
	class ENGINE_CORE_API ResourceManager : public Abstracts::Singleton<ResourceManager>
	{
		GENERATE_BODY
	public:
		explicit ResourceManager(SINGLETON_LOCK_TOKEN) {}

		void Initialize() override;

#if WITH_EDITOR
		void OnUIUpdate(UIContext* const parent, const float dt) override;
#endif
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void PreRender(const float dt) override;
		void PostUpdate(const float dt) override;
		void Render(const float dt) override;
		void PostRender(const float dt) override;
		void FixedUpdate(const float dt) override;

		template <typename T> requires std::is_base_of_v<Abstracts::Resource, T> && !std::is_same_v<Abstracts::Resource, T>
		void AddResource(const Strong<T>& resource)
		{
			AddResource(resource, T::StaticTypeHash());
		}

		template <typename T> requires std::is_base_of_v<Abstracts::Resource, T> && !std::is_same_v<Abstracts::Resource, T>
		void AddResource(const std::string_view name, const Strong<T>& resource)
		{
			AddResource(name, resource, T::StaticTypeHash());
		}

		inline void AddResource(const std::string_view name, const Strong<Abstracts::Resource>& resource, const ResourceType type);
		inline void AddResource(const Strong<Abstracts::Resource>& resource, const ResourceType type);

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

		template <typename T> requires std::is_base_of_v<Abstracts::Resource, T> && !std::is_same_v<Abstracts::Resource, T>
		Weak<T> GetResourceByMetadataPath(const std::filesystem::path& path)
		{
			if (const Strong<Abstracts::Resource>& found = GetResourceByMetadataPath(path, T::StaticTypeHash()).lock()) 
			{
				return boost::reinterpret_pointer_cast<T>(found);
			}

			return {};
		}
		
		Weak<Abstracts::Resource> GetResourceByMetadataPath(const std::filesystem::path& path, ResourceType type);

#if WITH_EDITOR
		void RegisterLoadResource(const std::string_view name, const UIHelpers::ManagedBooleanSignature& functor);
		void UnregisterLoadResource(const std::string_view name);
		auto RegisterNewResource(const std::string_view name, const UIHelpers::ManagedBooleanSignature& functor) -> void;
		void UnregisterNewResource(const std::string_view name);
		
	private:
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

		template <typename T> requires std::is_base_of_v<Abstracts::Resource, T> && !std::is_same_v<Abstracts::Resource, T>
		Weak<Abstracts::Resource> SearchResourceByMetadata(const std::filesystem::path& path)
		{
			return SearchResourceByMetadata(path, T::StaticTypeHash());
		}

		Weak<Abstracts::Resource> SearchResourceByMetadata(const std::filesystem::path& path, ResourceType type) const;

#if WITH_EDITOR
	public:
		const ResourceMap& GetResources() const;
#endif
	};
} // namespace Engine::Managers
