// Client.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "Client.h"

#include <egCubeMesh.h>
#include <egSphereMesh.h>
#include <egFont.h>
#include <egNormalMap.h>
#include <egResourceManager.h>
#include <egSceneManager.hpp>
#include <egSerialization.hpp>
#include <egSound.h>
#include <egSphereMesh.h>
#include "clBackSphereMesh.hpp"
#include "clTestScene.hpp"
#include "framework.h"

// TODO: This is an example of a library function
namespace Client
{
    void fnClient()
    {
        Engine::GetResourceManager().AddResource(
                                                 "TestTexture",
                                                 boost::make_shared<Engine::Resources::Texture>("./Texture.png"));
        Engine::GetResourceManager().AddResource(
                                                 "Sky", boost::make_shared<Engine::Resources::Texture>("./Sky.jpg"));
        Engine::GetResourceManager().AddResource(
                                                 "TestNormalMap",
                                                 boost::make_shared<Engine::Resources::NormalMap>(
                                                  "./Texture-Normal.png"));

        Engine::GetResourceManager().AddResource(
                                                 "TriangleMesh", boost::make_shared<Mesh::TriangleMesh>());
        Engine::GetResourceManager().AddResource(
                                                 "CubeMesh", boost::make_shared<Engine::Mesh::CubeMesh>());
        Engine::GetResourceManager().AddResource(
                                                 "SphereMesh", boost::make_shared<Engine::Mesh::SphereMesh>());

        Engine::GetResourceManager().AddResource(
                                                 "WaterNormal",
                                                 boost::make_shared<Engine::Resources::NormalMap>(
                                                  "./Water-Normal.png"));

        Engine::GetResourceManager().AddResource(
                                                 "BackSphereMesh", boost::make_shared<Mesh::BackSphereMesh>());
        Engine::GetResourceManager().AddResource(
                                                 "DefaultFont",
                                                 boost::make_shared<Engine::Resources::Font>("./consolas.spritefont"));
        Engine::GetResourceManager().AddResource(
                                                 "AmbientSound", boost::make_shared<Engine::Resources::Sound>(
                                                  "./"
                                                  "crowded-avenue-people-talking-vendors-shouting-"
                                                  "musicians-playing.mp3"));

        Engine::GetSceneManager().AddScene<Scene::TestScene>();
        Engine::GetSceneManager().SetActive<Scene::TestScene>();
    }
} // namespace Client
