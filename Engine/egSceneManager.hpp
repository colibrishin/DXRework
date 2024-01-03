#pragma once
#include "egCommon.hpp"
#include "egDebugger.hpp"
#include "egHelper.hpp"
#include "egManager.hpp"
#include "egScene.hpp"
#include "egShadowManager.hpp"

namespace Engine::Manager
{
    class SceneManager final : public Abstract::Singleton<SceneManager>
    {
    public:
        explicit SceneManager(SINGLETON_LOCK_TOKEN)
        : Singleton() {}

        WeakScene GetActiveScene() const
        {
            return m_active_scene_;
        }

        template <typename T, typename... Args, typename SceneLock = std::enable_if_t<std::is_base_of_v<Scene, T>>>
        void AddScene(Args&&... args)
        {
            auto scene = boost::make_shared<T>(std::forward<Args>(args)...);
            scene->Initialize();
            m_scenes_[which_scene<T>::value] = scene;
        }

        void ChangeScene(const WeakScene& it);

        template <typename T>
        void SetActive()
        {
            if (m_scenes_.contains(which_scene<T>::value))
            {
                ChangeScene(m_scenes_[which_scene<T>::value]);
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
                const auto scene = m_scenes_[which_scene<T>::value];

                if (scene == m_active_scene_.lock())
                {
                    Debugger::GetInstance().Log(L"Warning: Active scene has been removed.");
                    Graphics::ShadowManager::GetInstance().Reset();
                    m_active_scene_.reset();
                }

                m_scenes_.erase(which_scene<T>::value);
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
        WeakScene                         m_active_scene_;
        std::map<eSceneType, StrongScene> m_scenes_;
    };
} // namespace Engine::Manager
