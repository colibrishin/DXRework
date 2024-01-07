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

        WeakScene GetActiveScene() const
        {
            return m_active_scene_;
        }

        template <typename T, typename... Args, typename SceneLock = std::enable_if_t<std::is_base_of_v<Scene, T>>>
        void AddScene(Args&&... args)
        {
            auto scene = boost::make_shared<T>(std::forward<Args>(args)...);
            m_scenes_[which_scene<T>::value] = scene;
        }

        template <typename T>
        void SetActive()
        {
            if (const auto current = m_active_scene_.lock())
            {
                if (current->GetType() == which_scene<T>::value)
                {
                    return;
                }
            }

            // Orders :
            // 1. At the start of the frame, scene will be set as active and initialized, resetting shadow manager, push back to the task queue if there is any object creation, which has the higher priority than the scene. it will be processed in the next frame.
            // 2. Scene is activated, passing through the first frame without any objects. This has the effect that averaging out the noticeable delta time spike.
            // 3. On the second frame, pushed objects are processed, and added to the scene.
            // 4. Scene InitializeFinalize is called, and the scene loading is finished.
            if (m_scenes_.contains(which_scene<T>::value))
            {
                GetTaskScheduler().AddTask(
                                           TASK_ACTIVE_SCENE,
                                           {},
                                           [this](const std::vector<std::any>&, float)
                                           {
                                               SetActiveFinalize(m_scenes_[which_scene<T>::value]);
                                           });
            }
        }

        template <typename T>
        std::weak_ptr<T> GetScene() const
        {
            if (m_scenes_.contains(which_scene<T>::value))
            {
                return m_scenes_[which_scene<T>::value];
            }

            return {};
        }

        template <typename T>
        void RemoveScene()
        {
            if (m_scenes_.contains(which_scene<T>::value))
            {
                GetTaskScheduler().AddTask(
                                           TASK_REM_SCENE,
                                           {},
                                           [this](const std::vector<std::any>&, float)
                                           {
                                               RemoveSceneFinalize<T>();
                                           });
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

    private:
        friend struct SingletonDeleter;
        ~SceneManager() override = default;

        void SetActiveFinalize(const WeakScene& it);

        template <typename T>
        void RemoveSceneFinalize()
        {
            const auto scene = m_scenes_[which_scene<T>::value];

            if (scene == m_active_scene_.lock())
            {
                Debugger::GetInstance().Log("Warning: Active scene has been removed.");
                Graphics::ShadowManager::GetInstance().Reset();
                m_active_scene_.reset();
            }

            m_scenes_.erase(which_scene<T>::value);
        }

        WeakScene                         m_active_scene_;
        std::map<eSceneType, StrongScene> m_scenes_;
    };
} // namespace Engine::Manager
