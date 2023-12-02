// Client.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "Client.h"

#include "clBackSphereMesh.hpp"
#include "clGiftBox.hpp"
#include "clTestScene.hpp"
#include "framework.h"
#include "../Engine/egFont.hpp"
#include "../Engine/egResourceManager.hpp"
#include "../Engine/egSceneManager.hpp"
#include "../Engine/egCubeMesh.hpp"
#include "../Engine/egSphereMesh.hpp"
#include "egSound.hpp"

// TODO: This is an example of a library function
namespace Client
{
	void fnClient()
	{
		Engine::GetResourceManager().AddResource<Engine::Resources::Texture>(L"TestTexture", boost::make_shared<Engine::Resources::Texture>(L"./Texture.png"));
		Engine::GetResourceManager().AddResource<Engine::Resources::Texture>(L"Sky", boost::make_shared<Engine::Resources::Texture>(L"./Sky.jpg"));
		Engine::GetResourceManager().AddResource<Engine::Resources::NormalMap>(L"TestNormalMap", boost::make_shared<Engine::Resources::NormalMap>(L"./Texture-Normal.png"));
		Engine::GetResourceManager().AddResource<Engine::Resources::Mesh>(L"TriangleMesh", boost::make_shared<Client::Mesh::TriangleMesh>());
		Engine::GetResourceManager().AddResource<Engine::Resources::Mesh>(L"Giftbox", boost::make_shared<Client::Mesh::GiftBox>());
		Engine::GetResourceManager().AddResource<Engine::Resources::Mesh>(L"CubeMesh", boost::make_shared<Engine::Mesh::CubeMesh>());
		Engine::GetResourceManager().AddResource<Engine::Resources::Mesh>(L"SphereMesh", boost::make_shared<Engine::Mesh::SphereMesh>());

		Engine::GetResourceManager().AddResource<Engine::Resources::Mesh>(L"BackSphereMesh", boost::make_shared<Client::Mesh::BackSphereMesh>());
		Engine::GetResourceManager().AddResource<Engine::Resources::Font>(L"DefaultFont", boost::make_shared<Engine::Resources::Font>("./consolas.spritefont"));
		Engine::GetResourceManager().AddResource<Engine::Resources::Sound>(L"AmbientSound", boost::make_shared<Engine::Resources::Sound>("./crowded-avenue-people-talking-vendors-shouting-musicians-playing.mp3"));

		Engine::GetSceneManager().AddScene<Scene::TestScene>();
		Engine::GetSceneManager().SetActive<Scene::TestScene>();
	}
}

