#pragma once
#include <vector>
#include <functional>

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

	private:
		std::vector<std::function<void()>> m_tasks_;

	};

	inline void TaskScheduler::Initialize()
	{
	}

	inline void TaskScheduler::PreUpdate(const float& dt)
	{
	}

	inline void TaskScheduler::Update(const float& dt)
	{
		for (const auto& f : m_tasks_)
		{
			f();
		}

		m_tasks_.clear();
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
