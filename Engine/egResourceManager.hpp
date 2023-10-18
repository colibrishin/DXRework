#pragma once
#include "egManager.hpp"
#include "egResource.hpp"

namespace Engine::Manager
{
	class ResourceManager : public Abstract::Manager
	{
	public:
		void Initialize() override;
		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Render() override;

		static ResourceManager* GetInstance()
		{
			if (!m_instance_)
			{
				m_instance_ = std::unique_ptr<ResourceManager>(new ResourceManager);
				m_instance_->Initialize();
			}

			return m_instance_.get();
		}

		template <typename T>
		static void AddResource(const std::wstring& name, const std::shared_ptr<T>& resource)
		{
			m_resources_.insert(resource);
			resource->SetName(name);
		}

		template <typename T>
		static std::weak_ptr<T> GetResource(const std::wstring& name)
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
				return std::dynamic_pointer_cast<T>(*it);
			}

			return {};
		}

	private:
		ResourceManager() = default;

		inline static std::set<std::shared_ptr<Abstract::Resource>> m_resources_ = {};
		inline static std::unique_ptr<ResourceManager> m_instance_ = nullptr;
	};

	inline void ResourceManager::Initialize()
	{
	}

	inline void ResourceManager::PreUpdate()
	{
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
}
