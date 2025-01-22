#pragma once
#include "CameraManager/Public/CameraManager.h"

#include "Debugger/Public/Debugger.h"
#include "ModuleManager/Public/IModule.h"
#include "ResourceManager/Public/ResourceManager.h"
#include "SceneManager/Public/SceneManager.h"

#include "CoreModule.generated.h"

using SingletonCollection = std::vector<Engine::Abstracts::SingletonBase&(*)()>;

#define UPDATE_CALL_TEMPLATE(UpdateType) \
	void Do##UpdateType(const float dt, const SingletonCollection& singletons) \
	{ \
		for (const auto& s : singletons) \
		{ \
			s().##UpdateType(dt); \
		} \
	}

#define UPDATE_CALL_TEMPLATE_OneParam(UpdateType, ParamType, ParamName) \
	void Do##UpdateType(##ParamType ParamName##, const float dt, const SingletonCollection& singletons) \
	{ \
		for (const auto& s : singletons) \
		{ \
			s().##UpdateType(##ParamName, dt); \
		} \
	}

UPDATE_CALL_TEMPLATE_OneParam(OnUIUpdate, Engine::UIContext* const, parent)
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

	struct ENGINE_CORE_API CoreLoop
	{
		INLINE_COMPILE_TIME_TYPENAME_NON_ENTITY(CoreLoop)

		enum ENGINE_CORE_API eLoopType
		{
			LOOP_TYPE_RENDER,
			LOOP_TYPE_LOGIC,
			LOOP_TYPE_PHYSICS,
			LOOP_TYPE_MAX
		};

		void OnUIUpdate(UIContext* const parent, const float dt) const;
		void PreUpdate(const float dt) const;
		void Update(const float dt) const;
		void PostUpdate(const float dt) const;
		void FixedUpdate(const float dt) const;
		void PreRender(const float dt) const;
		void Render(const float dt) const;
		void PostRender(const float dt) const;
		
		template <typename... Args>
		void AddManager(const eLoopType loop_type, Args&&... args)
		{
			(args().Initialize(), ...);
			(m_singleton_accessor_[loop_type].push_back(reinterpret_cast<Abstracts::SingletonBase&(*&&)()>(args)), ...);
		}

		template <typename... Args>
		void RemoveManager(const eLoopType loop_type, Args&&... args)
		{
			(args().Destroy(), ...);

			std::apply([&](const auto&... ptrs)
			{
				const auto& removeFromArray = [&](const auto& ptr)
				{
					for (auto it = m_singleton_accessor_[loop_type].begin(); it != m_singleton_accessor_[loop_type].end();)
					{
						if (*it == reinterpret_cast<Abstracts::SingletonBase&(*)()>(ptr))
						{
							it = m_singleton_accessor_[loop_type].erase(it);
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
		std::vector<Abstracts::SingletonBase&(*)()> m_singleton_accessor_[LOOP_TYPE_MAX];
	};
}

namespace Engine
{
	ECLASS(module)
	struct ENGINE_CORE_API CoreModule : public IModule
	{
		GENERATE_BODY

		void Initialize() override;
		void Shutdown() override;
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

