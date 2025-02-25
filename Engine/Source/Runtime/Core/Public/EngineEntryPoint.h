#pragma once
#include "Singleton.h"
#include "StepTimer.hpp"

#include "EngineEntryPoint.generated.h"

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


namespace Engine
{
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

#if WITH_EDITOR
        static void OnUIUpdate(UIContext* const parent, const float dt);
#endif
        static void PreUpdate(const float dt);
        static void Update(const float dt);
        static void PostUpdate(const float dt);
        static void FixedUpdate(const float dt);
        static void PreRender(const float dt);
        static void Render(const float dt);
        static void PostRender(const float dt);
		
        template <typename... Args>
        static void AddManager(const eLoopType loop_type, Args&&... args)
        {
            (args().Initialize(), ...);
            (m_singleton_accessor_[loop_type].push_back(reinterpret_cast<Abstracts::SingletonBase&(*&)()>(args)), ...);
        }

        template <typename... Args>
        static void RemoveManager(const eLoopType loop_type, Args&&... args)
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
        static SingletonCollection m_singleton_accessor_[LOOP_TYPE_MAX];
    };
}

namespace Engine::Managers
{
	ECLASS()
	class ENGINE_CORE_API EngineEntryPoint final : public Abstracts::Singleton<EngineEntryPoint>
	{
		GENERATE_BODY
	public:
		EngineEntryPoint(SINGLETON_LOCK_TOKEN);

		void        Initialize() override;
		void        Tick();

		float           GetDeltaTime() const;
		uint32_t        GetFPS() const;

	private:
		friend struct SingletonDeleter;
		~EngineEntryPoint() override;

#if WITH_EDITOR
		void OnUIUpdate(UIContext* const parent, const float dt) override;
#endif

		void PreUpdate(const float dt) override;
		void FixedUpdate(const float dt) override;
		void Update(const float dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void PostRender(const float dt) override;
		void PostUpdate(const float dt) override;

		void tickInternal();

		static void SIGTERM();

		HWND m_hWnd = nullptr;

		// Time
		std::unique_ptr<DX::StepTimer> m_timer;

		// Check for Sigterm registration
		static bool s_instantiated_;
		static std::atomic<bool> s_paused;
		static std::atomic<float> s_fixed_update_interval;
	};
} // namespace Engine::Manager
