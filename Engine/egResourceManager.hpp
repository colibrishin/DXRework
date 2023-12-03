#pragma once
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

		template <typename T>
		static void AddResource(const std::wstring& name, const boost::shared_ptr<T>& resource)
		{
			m_resources_.insert(resource);
			resource->SetName(name);
		}

		template <typename T>
		static boost::weak_ptr<T> GetResource(const std::wstring& name)
		{
			if constexpr (std::is_base_of_v<Abstract::Resource, T>)
			{
				auto it = std::find_if(
				m_resources_.begin(),
				m_resources_.end(),
				[&name](const auto& resource)
				{
					if (const auto ptr = boost::dynamic_pointer_cast<T>(resource))
					{
						return ptr->GetName() == name;
					}

					return false;
				}
				);

				if (it != m_resources_.end())
				{
					(*it)->Load();
					return boost::dynamic_pointer_cast<T>(*it);
				}
			}

			return {};
		}

		static WeakResource GetResource(const std::wstring& name)
		{
			const auto it = std::ranges::find_if(m_resources_
			                               ,
			                               [&name](const auto& resource)
			                               {
				                               return resource->GetName() == name;
			                               }
			);

			if (it != m_resources_.end())
			{
				(*it)->Load();
				return *it;
			}

			return {};
		}

	private:
		inline static std::set<StrongResource> m_resources_ = {};

	};

	inline void ResourceManager::Initialize()
	{
		for (const auto& resource : m_resources_)
		{
			if (resource->weak_from_this().use_count() == 0 && resource->IsLoaded())
			{
				resource->Unload();
			}
			else if (resource->weak_from_this().use_count() != 0 && !resource->IsLoaded())
			{
				resource->Load();
			}
		}
	}

	inline void ResourceManager::PreUpdate(const float& dt)
	{
		for (const auto& resource : m_resources_)
		{
			if (resource->weak_from_this().use_count() == 0 && resource->IsLoaded())
			{
				resource->Unload();
			}
			else if (resource->weak_from_this().use_count() != 0 && !resource->IsLoaded())
			{
				resource->Load();
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
