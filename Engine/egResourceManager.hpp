#pragma once
#include "egManager.hpp"
#include "egResource.hpp"

namespace Engine::Manager
{
	class ResourceManager : public Abstract::Manager<ResourceManager>
	{
	public:
		explicit ResourceManager(SINGLETON_LOCK_TOKEN) : Manager() {}

		void Initialize() override;
		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Render() override;
		void FixedUpdate() override;

		template <typename T>
		static void AddResource(const std::wstring& name, const std::shared_ptr<T>& resource)
		{
			m_resources_.insert(resource);
			resource->SetName(name);
		}

		template <typename T>
		static std::weak_ptr<T> GetResource(const std::wstring& name)
		{
			if constexpr (std::is_base_of_v<Abstract::Resource, T>)
			{
				auto it = std::find_if(
				m_resources_.begin(),
				m_resources_.end(),
				[&name](const auto& resource)
				{
					if (const auto ptr = std::dynamic_pointer_cast<T>(resource))
					{
						return ptr->GetName() == name;
					}

					return false;
				}
				);

				if (it != m_resources_.end())
				{
					(*it)->Load();
					return std::dynamic_pointer_cast<T>(*it);
				}
			}

			return {};
		}

	private:
		inline static std::set<std::shared_ptr<Abstract::Resource>> m_resources_ = {};

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

	inline void ResourceManager::PreUpdate()
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

	inline void ResourceManager::Update()
	{
	}

	inline void ResourceManager::PreRender()
	{
	}

	inline void ResourceManager::Render()
	{
	}

	inline void ResourceManager::FixedUpdate()
	{
	}
}
