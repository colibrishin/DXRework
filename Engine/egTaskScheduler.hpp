#pragma once
#include <queue>
#include <functional>
#include "egManager.hpp"

namespace Engine::Manager
{
	class TaskScheduler : public Abstract::Singleton<TaskScheduler>
	{
	public:
		TaskScheduler(SINGLETON_LOCK_TOKEN) : Singleton() {}
		~TaskScheduler() override = default;

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void FixedUpdate(const float& dt) override;

		void AddTask(const TaskSchedulerFunc& task)
		{
			m_tasks_.push(task);
		}

	private:
		std::queue<TaskSchedulerFunc> m_tasks_;

	};

	inline void TaskScheduler::Initialize()
	{
	}

	inline void TaskScheduler::PreUpdate(const float& dt)
	{
	}

	inline void TaskScheduler::Update(const float& dt)
	{
		while (!m_tasks_.empty())
		{
			m_tasks_.front()(dt);
			m_tasks_.pop();
		}
	}

	inline void TaskScheduler::PreRender(const float& dt)
	{
	}

	inline void TaskScheduler::Render(const float& dt)
	{
	}

	inline void TaskScheduler::FixedUpdate(const float& dt)
	{
	}
}
