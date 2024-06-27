#pragma once
#include "egCommon.hpp"
#include "egDebugger.hpp"
#include "egHelper.hpp"
#include "egScene.hpp"
#include "egShadowManager.hpp"

namespace Engine::Manager
{
  class SceneManager final : public Abstract::Singleton<SceneManager>
  {
  public:
    explicit SceneManager(SINGLETON_LOCK_TOKEN) {}

    WeakScene GetActiveScene() const { return m_active_scene_; }

    void AddScene(const std::string& name)
    {
      const auto scene = boost::make_shared<Scene>();
      scene->SetName(name);
      m_scenes_.push_back(scene);
    }

    void SetActive(const std::string& name)
    {
      // Orders :
      // 1. At the start of the frame, scene will be set as active and initialized, resetting shadow manager, push back to the task queue if there is any object creation, which has the higher priority than the scene. it will be processed in the next frame.
      // 2. Scene is activated, passing through the first frame without any objects. This has the effect that averaging out the noticeable delta time spike.
      // 3. On the second frame, pushed objects are processed, and added to the scene.
      if (const auto scene = std::ranges::find_if(m_scenes_, [name](const auto&scene)
      {
        return scene->GetName() == name;
      }); 
          scene != m_scenes_.end())
      {
        GetTaskScheduler().AddTask
          (
           TASK_ACTIVE_SCENE,
           {*scene},
           [name, this](const std::vector<std::any>& params, float)
           {
             const auto s = std::any_cast<StrongScene>(params[0]);
             SetActiveFinalize(s);
           }
          );
      }
    }

    WeakScene GetScene(const std::string& name) const
    {
      const auto                       scene = std::ranges::find_if
        (m_scenes_, [name](const auto& scene)
        {
          return scene->GetName() == name;
        });

      if (scene != m_scenes_.end()) { return *scene; }

      return {};
    }

    template <typename T>
    void RemoveScene(const std::string& name)
    {
      if (const auto scene = std::ranges::find_if(m_scenes_, [name](const auto& scene)
      {
        return scene->GetName() == name;
      }); 
          scene != m_scenes_.end())
      {
        GetTaskScheduler().AddTask
          (
           TASK_REM_SCENE,
           {*scene, name},
           [this](const std::vector<std::any>& params, float)
           {
             const auto scene = std::any_cast<StrongScene>(params[0]);
             const auto name = std::any_cast<std::string>(params[1]);
             RemoveSceneFinalize(scene, name);
           }
          );
      }
    }

    void Initialize() override;
    void Update(const float& dt) override;
    void PreUpdate(const float& dt) override;
    void PreRender(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void Render(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PostRender(const float& dt) override;
    void OnImGui() override;

  private:
    friend struct SingletonDeleter;
    ~SceneManager() override = default;

    // Internal usage of add scene, used for un-deducible type (runtime).
    void AddScene(const WeakScene& ptr_scene);

    void SetActiveFinalize(const WeakScene& it);
    void OpenLoadPopup();

    void RemoveSceneFinalize(const StrongScene& scene, const std::string& name)
    {
      if (scene == m_active_scene_.lock())
      {
        Debugger::GetInstance().Log("Warning: Active scene has been removed.");
        Graphics::ShadowManager::GetInstance().Reset();
        m_active_scene_.reset();
      }

      std::erase_if(m_scenes_, [scene](const auto& v_scene)
      {
        return scene == v_scene;
      });
    }

    bool m_b_load_popup_ = false;

    WeakScene                m_active_scene_;
    std::vector<StrongScene> m_scenes_;
  };
} // namespace Engine::Manager


REGISTER_TYPE(Engine::Manager::Application, Engine::Manager::SceneManager)