#include "pch.hpp"
#include "egEditorScene.hpp"

namespace Engine
{
	void EditorScene::Initialize()
	{
		Scene::Initialize();
	}

	void EditorScene::PreUpdate(const float& dt)
	{
		Scene::PreUpdate(dt);
	}

	void EditorScene::Update(const float& dt)
	{
		Scene::Update(dt);
	}

	void EditorScene::PreRender(const float dt)
	{
		Scene::PreRender(dt);
	}

	void EditorScene::Render(const float dt)
	{
		Scene::Render(dt);
	}

	void EditorScene::FixedUpdate(const float& dt)
	{
		Scene::FixedUpdate(dt);
	}

	void EditorScene::OnDeserialized()
	{
		Scene::OnDeserialized();
		throw std::exception("Editor scene is not meant to deserialized.");
	}

	void EditorScene::EditorScreen()
	{
		ImGui::PopItemWidth();
		ImGui::End();
	}
}
