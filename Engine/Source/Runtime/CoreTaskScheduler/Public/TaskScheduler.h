#pragma once
#include <any>
#include <functional>
#include <map>
#include <numeric>
#include <queue>

#include "CoreType.h"
#include "Singleton.h"

#include "TaskScheduler.generated.h"

namespace Engine
{
	using TaskSchedulerFunc = std::function<void(const std::vector<std::any>&, float)>;
	
	enum ENGINE_CORETASKSCHEDULER_API eTaskType : uint8_t
	{
		TASK_NONE = 0,
		TASK_TOGGLE_RASTER,

		TASK_CHANGE_LAYER,
		TASK_ADD_OBJ,
		TASK_ADD_CHILD,
		TASK_ADD_COMPONENT,

		TASK_CACHE_COMPONENT,
		TASK_UNCACHE_COMPONENT,

		TASK_TF_UPDATE,

		TASK_REM_COMPONENT,
		TASK_REM_CHILD,
		TASK_REM_OBJ,

		TASK_SYNC_SCENE,
		TASK_INIT_SCENE,
        TASK_REM_SCENE,
        TASK_CLONE_SCENE,
        TASK_ACTIVE_SCENE,
        TASK_PLAY_SCENE,
        TASK_STOP_SCENE,
        TASK_SCRIPT_EVENT,
		TASK_MAX
	};
}

namespace Engine::Managers
{
	ECLASS()
	class ENGINE_CORETASKSCHEDULER_API TaskScheduler : public Abstracts::Singleton<TaskScheduler>
	{
		GENERATE_BODY
	public:
		struct TaskValue
		{
			eTaskType             type;
			TaskSchedulerFunc     func;
			std::vector<std::any> params;
		};

		TaskScheduler(SINGLETON_LOCK_TOKEN)
			: Singleton() {}

		void Initialize() override;
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void PostRender(const float dt) override;
		void PostUpdate(const float dt) override;
		void FixedUpdate(const float dt) override;

		void AddTask(const eTaskType type, const std::vector<std::any>& params, const TaskSchedulerFunc& func)
		{
			m_tasks_[type].push
					(
					 {
						 type,
						 func,
						 params
					 }
					);
		}

	    void AddTask(const eTaskType type, const std::vector<std::any>& params)
		{
		    m_tasks_[type].push
                    (
                     {
                         type,
                         {},
                         params
                     }
                    );
		}

	    void Inject(const eTaskType type, const TaskSchedulerFunc& func);
	    void Extract(const eTaskType type, const TaskSchedulerFunc& func);

	private:
		TaskScheduler() = default;
		friend struct SingletonDeleter;
		~TaskScheduler() override = default;

		std::map<eTaskType, std::queue<TaskValue>> m_tasks_;
	    std::map<eTaskType, std::vector<TaskSchedulerFunc>> m_injected_funcs_;
	};
} // namespace Engine::Managers
