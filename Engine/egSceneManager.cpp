#include "pch.h"
#include "egSceneManager.hpp"
#include "egCamera.h"
#include "egLight.h"
#include "egMaterial.h"
#include "egObject.hpp"
#include "egScene.hpp"
#include "egShadowManager.hpp"
#include "egText.h"

namespace Engine::Manager
{
	void SceneManager::SetActiveFinalize(const WeakScene& it)
	{
		Graphics::ShadowManager::GetInstance().Reset();
		m_active_scene_ = it;

		if (const auto& scene = m_active_scene_.lock())
		{
			if (!it.lock()->IsInitialized())
			{
				scene->Initialize();
			}

			for (const auto& light :
			     scene->GetGameObjects(LAYER_LIGHT))
			{
				Graphics::ShadowManager::GetInstance().RegisterLight
						(
						 light.lock()->GetSharedPtr<Objects::Light>()
						);
			}

			g_raytracing = scene->m_b_scene_raytracing_;
		}
	}

	void SceneManager::OpenLoadPopup()
	{
		if (m_b_load_popup_)
		{
			if (ImGui::Begin
				(
				 "Load Scene", nullptr,
				 ImGuiWindowFlags_AlwaysAutoResize
				))
			{
				static char buf[256] = {0};

				ImGui::InputText("Filename", buf, IM_ARRAYSIZE(buf));

				if (ImGui::Button("Load"))
				{
					const auto scene = Serializer::Deserialize<Scene>(buf);
					AddScene(scene);
					SetActive(scene->GetName());

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
		AddScene("UntitledScene");
		SetActive("UntitledScene");
	}

	void SceneManager::Update(const float& dt)
	{
		if (const auto& scene = m_active_scene_.lock())
		{
			scene->Update(dt);
		}
	}

	void SceneManager::PreUpdate(const float& dt)
	{
		if (const auto& scene = m_active_scene_.lock())
		{
			scene->PreUpdate(dt);
		}
	}

	void SceneManager::PreRender(const float& dt)
	{
		if (const auto& scene = m_active_scene_.lock())
		{
			scene->PreRender(dt);
		}
	}

	void SceneManager::PostUpdate(const float& dt)
	{
		if (const auto& scene = m_active_scene_.lock())
		{
			scene->PostUpdate(dt);
		}
	}

	void SceneManager::Render(const float& dt)
	{
		if (const auto& scene = m_active_scene_.lock())
		{
			scene->Render(dt);
		}
	}

	void SceneManager::FixedUpdate(const float& dt)
	{
		if (const auto& scene = m_active_scene_.lock())
		{
			scene->FixedUpdate(dt);
		}
	}

	void SceneManager::PostRender(const float& dt)
	{
		if (const auto& scene = m_active_scene_.lock())
		{
			scene->PostRender(dt);
		}
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
			if (ImGui::BeginMenu("New"))
			{
				if (ImGui::MenuItem("Scene"))
				{
					AddScene("UntitledScene");
					SetActive("UntitledScene");
				}

				if (ImGui::MenuItem("Material"))
				{
					const auto mtr       = Resources::Material::Create("UntitledMaterial", "");
					mtr->IsImGuiOpened() = true;
				}

				ImGui::EndMenu();
			}

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

				if (ImGui::MenuItem("Object"))
				{
					GetActiveScene().lock()->CreateGameObject<Object>(LAYER_DEFAULT);
				}

				if (ImGui::MenuItem("Text"))
				{
					GetActiveScene().lock()->CreateGameObject<Objects::Text>
							(
							 LAYER_UI,
							 GetResourceManager().GetResource<Resources::Font>("DefaultFont")
							);
				}

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

			ImGui::EndMainMenuBar();
		}

		if (m_b_load_popup_)
		{
			OpenLoadPopup();
		}
	}

	void SceneManager::AddScene(const WeakScene& ptr_scene)
	{
		if (const auto param_scene = ptr_scene.lock())
		{
			if (const auto target = std::ranges::find_if
						(
						 m_scenes_, [param_scene](const auto& v_s)
						 {
							 return v_s->GetName() == param_scene->GetName();
						 }
						);
				m_scenes_.end() != target)
			{
				GetTaskScheduler().AddTask
						(
						 TASK_SYNC_SCENE,
						 {*target, param_scene},
						 [this](const std::vector<std::any>& params, float)
						 {
							 const auto target = std::any_cast<StrongScene>(params[0]);
							 const auto scene  = std::any_cast<StrongScene>(params[1]);
							 target->synchronize(scene);
						 }
						);
			}
			else
			{
				m_scenes_.push_back(param_scene);
			}
		}
	}
} // namespace Engine::Manager
