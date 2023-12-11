#pragma once
#include "egCommon.hpp"
#include "egManager.hpp"
#include "egResource.hpp"

namespace Engine::Manager
{
	class ResourceManager : public Abstract::Singleton<ResourceManager>
	{
	public:
		explicit ResourceManager(SINGLETON_LOCK_TOKEN) : Singleton() {}

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void FixedUpdate(const float& dt) override;

		void AddResource(const StrongResource& resource)
		{
			m_resources_[resource->GetTypeName()].insert(resource);
		}

		void AddResource(const EntityName& name, const StrongResource& resource)
		{
			m_resources_[resource->GetTypeName()].insert(resource);
			resource->SetName(name);
		}

		template <typename T>
		boost::weak_ptr<T> GetResource(const EntityName& name)
		{
			if constexpr (std::is_base_of_v<Abstract::Resource, T>)
			{
				const auto target = m_resources_[typeid(T).name()];

				if (target.empty())
				{
					return {};
				}

				const auto it = std::ranges::find_if(
					target,
                   [&name](const auto& resource)
                   {
                       return resource->GetName() == name;
                   });

				if (it != target.end())
				{
					if (!(*it)->IsLoaded())
					{
						(*it)->Load();
					}
					return boost::dynamic_pointer_cast<T>(*it);
				}
			}

			return {};
		}

		WeakResource GetResource(const TypeName& type, const EntityName& name)
		{
			const auto target = m_resources_[type];

			if (target.empty())
			{
				return {};
			}

			const auto it = std::ranges::find_if(
				target,
               [&name](const auto& resource)
               {
                   return resource->GetName() == name;
               });

			if (it != target.end())
			{
				if (!(*it)->IsLoaded())
				{
					(*it)->Load();
				}
				return *it;
			}

			return {};
		}

	private:
		std::map<const std::string, std::set<StrongResource>> m_resources_;

	};

	inline void ResourceManager::Initialize()
	{
	}

	inline void ResourceManager::PreUpdate(const float& dt)
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

	inline void ResourceManager::Update(const float& dt)
	{
	}

	inline void ResourceManager::PreRender(const float& dt)
	{
	}

	inline void ResourceManager::Render(const float& dt)
	{
	}

	inline void ResourceManager::FixedUpdate(const float& dt)
	{
	}
}
