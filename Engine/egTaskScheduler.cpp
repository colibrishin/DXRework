#include "pch.h"
#include "egTaskScheduler.h"

namespace Engine::Manager
{
    void TaskScheduler::Initialize() {}

    void TaskScheduler::PreUpdate(const float& dt)
    {
        while (!m_tasks_.empty())
        {
            m_tasks_.front()(dt);
            m_tasks_.pop();
        }
    }

    void TaskScheduler::Update(const float& dt) {}

    void TaskScheduler::PreRender(const float& dt) {}

    void TaskScheduler::Render(const float& dt) {}

    void TaskScheduler::PostRender(const float& dt) {}

    void TaskScheduler::PostUpdate(const float& dt) {}

    void TaskScheduler::FixedUpdate(const float& dt) {}
}