#include "Source/Runtime/Core/TaskScheduler/Public/TaskScheduler.h"

namespace Engine::Managers
{
	void TaskScheduler::Initialize() {}

	void TaskScheduler::PreUpdate(const float& dt)
	{
		for (int i = 0; i < TASK_MAX; ++i)
		{
			while (!m_tasks_[static_cast<eTaskType>(i)].empty())
			{
				auto task = m_tasks_[static_cast<eTaskType>(i)].front();
				m_tasks_[static_cast<eTaskType>(i)].pop();
				task.func(task.params, dt);
			}
		}
	}

	void TaskScheduler::Update(const float& dt) {}

	void TaskScheduler::PreRender(const float& dt) {}

	void TaskScheduler::Render(const float& dt) {}

	void TaskScheduler::PostRender(const float& dt) {}

	void TaskScheduler::PostUpdate(const float& dt) {}

	void TaskScheduler::FixedUpdate(const float& dt) {}
}
