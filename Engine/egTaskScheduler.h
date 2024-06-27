#pragma once
#include <any>
#include <functional>
#include <numeric>
#include <queue>
#include "egManager.hpp"

namespace Engine::Manager
{
  class TaskScheduler : public Abstract::Singleton<TaskScheduler>
  {
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
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void FixedUpdate(const float& dt) override;

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

  private:
    friend struct SingletonDeleter;
    ~TaskScheduler() override = default;

    std::map<eTaskType, std::queue<TaskValue>> m_tasks_;
  };
} // namespace Engine::Manager

REGISTER_TYPE(Engine::Manager::Application, Engine::Manager::TaskScheduler)