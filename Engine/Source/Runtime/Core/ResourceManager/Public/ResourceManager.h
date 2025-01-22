#pragma once
#include <ranges>
#include "Source/Runtime/Core/Resource/Public/Resource.h"
#include "Source/Runtime/CoreSingleton/Public/Singleton.h"
#include "Source/Runtime/Core/Allocator/Public/Allocator.h"

#include "Serialization.hpp"

#include "ResourceManager.generated.h"

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
		using ManagedBooleanSignature = std::function<void(bool&)>;
		using NewResourceSignature = std::function<void()>;

		void RegisterLoadResource(const std::string_view name, const ManagedBooleanSignature& functor);
		void UnregisterLoadResource(const std::string_view name);
		void RegisterNewResource(const std::string_view name, const ManagedBooleanSignature& functor);
		void UnregisterNewResource(const std::string_view name);

		[[nodiscard]] bool RequestAddResourceDialog();
		void EndAddResourceDialog();
		[[nodiscard]] bool TryAddResourceDialog(std::vector<Strong<Abstracts::Resource>>& resource_to_load);

		using UICallbackSignature = std::function<void(UIContext* const)>;
		using NameAndPathConfirmCallbackSignature = std::function<void(const std::string&, const std::string&)>;
		using UICleanupCallbackSignature = std::function<void()>;

		// todo: refactoring, possible shared usage.
		template <bool UseName, bool UsePath>
		bool NamePathDialogTemplate(
			bool& flag, 
			const std::string_view title,
			const std::string_view confirm_button_label,
			const UICallbackSignature& ui_callback,
			const NameAndPathConfirmCallbackSignature& confirm_callback,
			const UICleanupCallbackSignature& cleanup_callback)
		{
			UIInterface& ui = UIInterfaceAccessor::GetInterface();

			static bool pressed = false;
			static std::string name{};
			static std::string path{};

			if (UIContext context = UIInterface::NewContext(ui.NewDialog({ this, title, flag })))
			{
				if constexpr (UseName) 
				{
					context |= ui.NewLabelAndText({ "Name", name, true });
				}
				
				if constexpr (UsePath) 
				{
					context |= ui.NewLabelAndText({ "Path", path, true });
				}

				if (ui_callback)
				{
					ui_callback(&context);
				}

				(context |= ui.NewButton({ confirm_button_label })).SetFunction([&]()
					{
						pressed = true;
						flag = false;
					});

				(context |= ui.NewButton({ "Cancel" })).SetFunction([&]()
					{
						flag = false;
					});
			}

			if (pressed)
			{
				if (confirm_callback)
				{
					confirm_callback(name, path);
				}
				if (cleanup_callback) 
				{
					cleanup_callback();
				}
				name = {};
				path = {};
				flag = false;
				pressed = false;
				return false;
			}

			if (!flag)
			{
				if (cleanup_callback)
				{
					cleanup_callback();
				}
				name = {};
				path = {};
				return false;
			}

			return true;
		}

		template <typename T>
		bool OpenLoadDialog(bool& flag, 
			const UICallbackSignature& ui_callback, 
			const NameAndPathConfirmCallbackSignature& load_callback,
			const UICleanupCallbackSignature& cleanup_callback)
		{
			static std::string title = "Load ";
			static std::once_flag initialized;
			std::call_once(initialized, []()
				{
					title += T::StaticTypeName();
				});

			return NamePathDialogTemplate<false, true>(flag, title, "Load", ui_callback, load_callback, cleanup_callback);
		}

		template <typename T>
		bool OpenNewDialog(bool& flag,
			const UICallbackSignature& ui_callback,
			const NameAndPathConfirmCallbackSignature& load_callback,
			const UICleanupCallbackSignature& cleanup_callback)
		{
			static std::string title = "New ";
			static std::once_flag initialized;
			std::call_once(initialized, []()
				{
					title += T::StaticTypeName();
				});

			return NamePathDialogTemplate<true, true>(flag, title, "Confirm", ui_callback, load_callback, cleanup_callback);
		}

	private:
		bool m_b_ui_add_resource_ = false;
		
		std::unordered_map<std::string_view, ManagedBooleanSignature> m_ui_load_functions_;
		std::unordered_map<std::string_view, bool> m_ui_load_functions_managing_;
		std::unordered_map<std::string_view, ManagedBooleanSignature> m_ui_new_functions_;
		std::unordered_map<std::string_view, bool> m_ui_new_functions_managing_;
#endif
	private:
		friend struct SingletonDeleter;
		~ResourceManager() override;

		fast_pool_unordered_map<ResourceType, fast_pool_set<Strong<Abstracts::Resource>>> m_resources_;
		fast_pool_unordered_map<LocalResourceID, Weak<Abstracts::Resource>> m_resource_cache_;
		fast_pool_unordered_map<LocalResourceID, GlobalEntityID> m_resource_ids_;
	};
} // namespace Engine::Managers
