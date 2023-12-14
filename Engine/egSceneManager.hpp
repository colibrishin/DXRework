#pragma once
#include "egCommon.hpp"
#include "egDebugger.hpp"
#include "egHelper.hpp"
#include "egManager.hpp"
#include "egScene.hpp"
#include "egTaskScheduler.hpp"

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

        template <typename T, typename... Args>
        void AddScene(Args&&... args)
        {
            auto scene = Instantiate<T>(std::forward<Args>(args)...);
            m_scenes_.insert(scene);
        }

        void ChangeScene(const WeakScene& it);

        template <typename T>
        void SetActive()
        {
            auto it =
                    std::find_if(
                                 m_scenes_.begin(), m_scenes_.end(), [](const auto& scene)
                                 {
                                     return boost::dynamic_pointer_cast<T>(scene) != nullptr;
                                 });

            if (it != m_scenes_.end())
            {
                ChangeScene(*it);
            }
        }

        WeakScene GetScene(EntityID id) const
        {
            auto it =
                    std::find_if(
                                 m_scenes_.begin(), m_scenes_.end(),
                                 [id](const auto& scene)
                                 {
                                     return scene->GetID() == id;
                                 });

            if (it != m_scenes_.end()) return *it;

            return {};
        }

        template <typename T>
        void RemoveScene()
        {
            auto it =
                    std::find_if(
                                 m_scenes_.begin(), m_scenes_.end(), [](const auto& scene)
                                 {
                                     return boost::dynamic_pointer_cast<T>(scene) != nullptr;
                                 });

            if (it != m_scenes_.end())
            {
                if (*it == m_active_scene_.lock())
                {
                    Debugger::GetInstance().Log(L"Warning: Active scene has been removed.");
                    Graphics::ShadowManager::GetInstance().Clear();
                    m_active_scene_.reset();
                }

                m_scenes_.erase(it);
            }
        }

        void Initialize() override;
        void Update(const float& dt) override;
        void PreUpdate(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PostRender(const float& dt) override;

    private:
        WeakScene             m_active_scene_;
        std::set<StrongScene> m_scenes_;
    };
} // namespace Engine::Manager
