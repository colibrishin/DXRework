#include "pch.h"
#include "egSceneManager.hpp"
#include "egObject.hpp"
#include "egScene.hpp"
#include "egShadowManager.hpp"

namespace Engine::Manager
{
    void SceneManager::ChangeScene(const WeakScene& it)
    {
        Graphics::ShadowManager::GetInstance().Clear();
        m_active_scene_ = it;

        for (const auto& light :
             m_active_scene_.lock()->GetGameObjects(LAYER_LIGHT))
        {
            Graphics::ShadowManager::GetInstance().RegisterLight(
                                                                 light.lock()->GetSharedPtr<Objects::Light>());
        }
    }

    void SceneManager::Initialize() {}

    void SceneManager::Update(const float& dt)
    {
        m_active_scene_.lock()->Update(dt);
    }

    void SceneManager::PreUpdate(const float& dt)
    {
        m_active_scene_.lock()->PreUpdate(dt);
    }

    void SceneManager::PreRender(const float& dt)
    {
        m_active_scene_.lock()->PreRender(dt);
    }

    void SceneManager::Render(const float& dt)
    {
        m_active_scene_.lock()->Render(dt);
    }

    void SceneManager::FixedUpdate(const float& dt)
    {
        m_active_scene_.lock()->FixedUpdate(dt);
    }

    void SceneManager::PostRender(const float& dt)
    {
        m_active_scene_.lock()->PostRender(dt);
    }
} // namespace Engine::Manager
