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
#include "egSerialization.hpp"


// TODO: This is an example of a library function
namespace Client
{
	void fnClient()
	{
		Engine::GetResourceManager().AddResource("TestTexture", boost::make_shared<Engine::Resources::Texture>("./Texture.png"));
		Engine::GetResourceManager().AddResource("Sky", boost::make_shared<Engine::Resources::Texture>("./Sky.jpg"));
		Engine::GetResourceManager().AddResource("TestNormalMap", boost::make_shared<Engine::Resources::NormalMap>("./Texture-Normal.png"));
		Engine::GetResourceManager().AddResource("TriangleMesh", boost::make_shared<Client::Mesh::TriangleMesh>());
		Engine::GetResourceManager().AddResource("Giftbox", boost::make_shared<Client::Mesh::GiftBox>());
		Engine::GetResourceManager().AddResource("CubeMesh", boost::make_shared<Engine::Mesh::CubeMesh>());
		Engine::GetResourceManager().AddResource("SphereMesh", boost::make_shared<Engine::Mesh::SphereMesh>());

		Engine::GetResourceManager().AddResource("BackSphereMesh", boost::make_shared<Client::Mesh::BackSphereMesh>());
		Engine::GetResourceManager().AddResource("DefaultFont", boost::make_shared<Engine::Resources::Font>("./consolas.spritefont"));
		Engine::GetResourceManager().AddResource("AmbientSound", boost::make_shared<Engine::Resources::Sound>("./crowded-avenue-people-talking-vendors-shouting-musicians-playing.mp3"));

		Engine::GetSceneManager().AddScene<Scene::TestScene>();
		Engine::GetSceneManager().SetActive<Scene::TestScene>();
	}
}

