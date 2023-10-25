// Client.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "Client.h"

#include "clCubeMesh.hpp"
#include "clGiftBox.hpp"
#include "clSphereMesh.hpp"
#include "clTestScene.hpp"
#include "framework.h"
#include "../Engine/egFont.hpp"
#include "../Engine/egResourceManager.hpp"
#include "../Engine/egSceneManager.hpp"

// TODO: This is an example of a library function
namespace Client
{
	void fnClient()
	{
		Engine::GetResourceManager()->AddResource<Engine::Resources::Texture>(L"TestTexture", std::make_shared<Engine::Resources::Texture>(L"./Texture.png"));
		Engine::GetResourceManager()->AddResource<Engine::Resources::NormalMap>(L"TestNormalMap", std::make_shared<Engine::Resources::NormalMap>(L"./Texture-Normal.png"));
		Engine::GetResourceManager()->AddResource<Engine::Resources::Mesh>(L"TriangleMesh", std::make_shared<Client::Mesh::TriangleMesh>());
		Engine::GetResourceManager()->AddResource<Engine::Resources::Mesh>(L"CubeMesh", std::make_shared<Client::Mesh::CubeMesh>());
		Engine::GetResourceManager()->AddResource<Engine::Resources::Mesh>(L"Giftbox", std::make_shared<Client::Mesh::GiftBox>());
		Engine::GetResourceManager()->AddResource<Engine::Resources::Mesh>(L"SphereMesh", std::make_shared<Client::Mesh::SphereMesh>());
		Engine::GetResourceManager()->AddResource<Engine::Resources::Font>(L"DefaultFont", std::make_shared<Engine::Resources::Font>("./consolas.spritefont"));

		Engine::GetSceneManager()->AddScene<Scene::TestScene>();
		Engine::GetSceneManager()->SetActive<Scene::TestScene>();
	}
}

