// Client.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "Client.h"
#include "clTestScene.hpp"
#include "framework.h"
#include "../Engine/egSceneManager.hpp"

// TODO: This is an example of a library function
namespace Client
{
	void fnClient()
	{
		Engine::Manager::SceneManager::GetInstance()->AddScene<Scene::TestScene>();
		Engine::Manager::SceneManager::GetInstance()->SetActive<Scene::TestScene>();
	}
}

