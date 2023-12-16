#pragma once
#include <functional>
#include <queue>
#include "egManager.hpp"

namespace Engine::Manager
{
    class TaskScheduler : public Abstract::Singleton<TaskScheduler>
    {
    public:
        TaskScheduler(SINGLETON_LOCK_TOKEN)
        : Singleton() {}

        ~TaskScheduler() override = default;

        void Initialize() override;
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;
        void FixedUpdate(const float& dt) override;

        void AddTask(const TaskSchedulerFunc& task)
        {
            m_tasks_.push(task);
        }

    private:
        std::queue<TaskSchedulerFunc> m_tasks_;
    };
} // namespace Engine::Manager
