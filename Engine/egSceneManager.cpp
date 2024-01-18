#include "pch.h"
#include "egSceneManager.hpp"
#include "egObject.hpp"
#include "egScene.hpp"
#include "egShadowManager.hpp"
#include "egCamera.h"
#include "egLight.h"

namespace Engine::Manager
{
    void SceneManager::SetActiveFinalize(const WeakScene& it)
    {
        Graphics::ShadowManager::GetInstance().Reset();
        m_active_scene_ = it;

        if (!it.lock()->IsInitialized())
        {
            m_active_scene_.lock()->Initialize();
        }

        for (const auto& light :
             m_active_scene_.lock()->GetGameObjects(LAYER_LIGHT))
        {
            Graphics::ShadowManager::GetInstance().RegisterLight(
                                                                 light.lock()->GetSharedPtr<Objects::Light>());
        }
    }

    void SceneManager::OpenLoadPopup()
    {
        if (m_b_load_popup_)
        {
            if (ImGui::Begin(
                             "Load Scene", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize))
            {
                static char buf[256] = {0};

                ImGui::InputText("Filename", buf, IM_ARRAYSIZE(buf));

                if (ImGui::Button("Load"))
                {
                    const auto scene = Serializer::Deserialize<Scene>(buf);

                    GetActiveScene().lock()->Synchronize(scene);

                    m_b_load_popup_ = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();

                if (ImGui::Button("Cancel"))
                {
                    m_b_load_popup_ = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::End();
            }
        }
    }

    void SceneManager::Initialize()
    {
        m_b_load_popup_ = false;
    }

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

    void SceneManager::PostUpdate(const float& dt)
    {
        m_active_scene_.lock()->PostUpdate(dt);
    }

    void SceneManager::Render(const float& dt)
    {
        m_active_scene_.lock()->Render(dt);
        this->OnImGui();
    }

    void SceneManager::FixedUpdate(const float& dt)
    {
        m_active_scene_.lock()->FixedUpdate(dt);
    }

    void SceneManager::PostRender(const float& dt)
    {
        m_active_scene_.lock()->PostRender(dt);
    }

    void SceneManager::OnImGui()
    {
        const auto scene = GetActiveScene().lock();

        if (!scene)
        {
            return;
        }

        scene->OnImGui();

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Add"))
            {
                if (ImGui::MenuItem("Camera"))
                {
                    GetActiveScene().lock()->CreateGameObject<Objects::Camera>(LAYER_CAMERA);
                }

                if (ImGui::MenuItem("Light"))
                {
                    GetActiveScene().lock()->CreateGameObject<Objects::Light>(LAYER_LIGHT);
                }

                GetActiveScene().lock()->AddCustomObject();

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Load"))
            {
                if (ImGui::MenuItem("Scene"))
                {
                    m_b_load_popup_ = true;
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Save"))
            {
                if (ImGui::MenuItem("Scene"))
                {
                    GetActiveScene().lock()->Save();
                }

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    }
} // namespace Engine::Manager
