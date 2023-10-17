// Client.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "Client.h"
#include "clTestScene.hpp"
#include "framework.h"
#include "../Engine/egResourceManager.hpp"
#include "../Engine/egSceneManager.hpp"

// TODO: This is an example of a library function
namespace Client
{
	void fnClient()
	{
		Engine::GetResourceManager()->AddResource<Engine::Resources::Texture>(L"TestTexture", std::make_shared<Engine::Resources::Texture>(L"./Texture.png"));
		Engine::GetResourceManager()->AddResource<Engine::Resources::Mesh>(L"TriangleMesh", std::make_shared<Client::Mesh::TriangleMesh>());

		Engine::GetSceneManager()->AddScene<Scene::TestScene>();
		Engine::GetSceneManager()->SetActive<Scene::TestScene>();
	}
}

