#pragma once
#include "ModuleManager/Public/IModule.h"
#include "ResourceManager/Public/ResourceManager.hpp"
#include "SceneManager/Public/SceneManager.hpp"

using SingletonCollection = std::vector<Engine::Abstracts::SingletonBase&(*)()>;

#define UPDATE_CALL_TEMPLATE(UpdateType) \
	void Do##UpdateType(const float dt, const SingletonCollection& singletons) \
	{ \
		for (const auto& s : singletons) \
		{ \
			s().##UpdateType(dt); \
		} \
	}

UPDATE_CALL_TEMPLATE(PreUpdate)
UPDATE_CALL_TEMPLATE(Update)
UPDATE_CALL_TEMPLATE(PostUpdate)
UPDATE_CALL_TEMPLATE(FixedUpdate)
UPDATE_CALL_TEMPLATE(PreRender)
UPDATE_CALL_TEMPLATE(Render)
UPDATE_CALL_TEMPLATE(PostRender)

namespace Engine
{
	namespace Managers
	{
		class ModuleManager;
	}

	struct CoreModule;

	struct CORE_API CoreLoop
	{
		void PreUpdate(const float dt) const;
		void Update(const float dt) const;
		void PostUpdate(const float dt) const;
		void FixedUpdate(const float dt) const;
		void PreRender(const float dt) const;
		void Render(const float dt) const;
		void PostRender(const float dt) const;
		
		template <typename... Args>
		void AddManager(Args&&... args)
		{
			(args().Initialize(), ...);
			(m_singleton_accessor_.push_back(reinterpret_cast<Abstracts::SingletonBase&(*&&)()>(args)), ...);
		}

		template <typename... Args>
		void RemoveManager(Args&&... args)
		{
			(args().Destroy(), ...);

			std::apply([&](const auto&... ptrs)
			{
				const auto& removeFromArray = [&](const auto& ptr)
				{
					for (auto it = m_singleton_accessor_.begin(); it != m_singleton_accessor_.end();)
					{
						if (*it == reinterpret_cast<Abstracts::SingletonBase&(*)()>(ptr))
						{
							it = m_singleton_accessor_.erase(it);
							break;
						}
						else
						{
							++it;
						}
					}
				};

				(removeFromArray(ptrs), ...);

			}, std::forward_as_tuple(args...));
		}

	private:
		std::vector<Abstracts::SingletonBase&(*)()> m_singleton_accessor_{};
	};
	
	struct CORE_API CoreModule : public IModule
	{
		void Initialize() override
		{
			s_core_module.AddManager(
				&Managers::ResourceManager::GetInstance, 
				&Managers::SceneManager::GetInstance, 
				&Managers::TaskScheduler::GetInstance);
		}

		void Shutdown() override
		{
			s_core_module.RemoveManager(
				&Managers::ResourceManager::GetInstance,
				&Managers::SceneManager::GetInstance,
				&Managers::TaskScheduler::GetInstance);
		}

		bool DynamicLoadable() override
		{
			return true;
		}

		static CoreLoop& GetContext()
		{
			return s_core_module;
		}
		
		static CoreLoop s_core_module;
	};
}
