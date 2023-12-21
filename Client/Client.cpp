// Client.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "Client.h"

#include <egCubeMesh.h>
#include <egSphereMesh.h>
#include <egFont.h>
#include <egNormalMap.h>
#include <egResourceManager.hpp>
#include <egSceneManager.hpp>
#include <egSerialization.hpp>
#include <egSound.h>
#include <egSphereMesh.h>
#include <egModel.h>

#include "clBackSphereMesh.hpp"
#include "clTestScene.hpp"
#include "framework.h"

// TODO: This is an example of a library function
namespace Client
{
    void InitializeTexture()
    {
        Engine::Resources::Texture::Create("TestTexture", "./Texture.dds");
        Engine::Resources::Texture::Create("Sky", "./Sky.dds");
    }

    void InitializeMesh()
    {
        Engine::GetResourceManager().AddResource(
                                                 "TriangleMesh", boost::make_shared<Meshes::TriangleMesh>());
        Engine::GetResourceManager().AddResource(
                                                 "CubeMesh", boost::make_shared<Engine::Meshes::CubeMesh>());
        Engine::GetResourceManager().AddResource(
                                                 "SphereMesh", boost::make_shared<Engine::Meshes::SphereMesh>());
        Engine::GetResourceManager().AddResource(
                                                 "BackSphereMesh", boost::make_shared<Meshes::BackSphereMesh>());
    }

    void InitializeNormal()
    {
        Engine::Resources::NormalMap::Create("TestNormalMap", "./Texture-Normal.dds");
        Engine::Resources::NormalMap::Create("WaterNormalMap", "./Water-Normal.dds");
    }

    void InitializeSound()
    {
        Engine::Resources::Sound::Create(
                                         "AmbientSound",
                                         "./crowded-avenue-people-talking-vendors-shouting-musicians-playing.mp3");
    }

    void InitializeFont()
    {
        Engine::Resources::Font::Create("DefaultFont", "./consolas.spritefont");
    }

    void InitializeModel()
    {
        Resources::Model::Create("BobModel", "./bob_lamp_update_export.fbx");
        Resources::Model::Create("PlayerModel", "./player.obj");

        std::vector<Engine::StrongResource> resources;

        resources.push_back(Engine::Resources::Mesh::Get("CubeMesh").lock());
        resources.push_back(Engine::GetResourceManager().GetResource<Engine::Resources::Texture>("TestTexture").lock());
        resources.push_back(Engine::GetResourceManager().GetResource<Engine::Resources::NormalMap>("TestNormalMap").lock());

        const auto cube_model = Engine::Resources::Model::Create("CubeModel", resources);

        GetResourceManager().AddResource(cube_model);

        resources.clear();

        resources.push_back(Engine::Resources::Mesh::Get("SphereMesh").lock());
        resources.push_back(Engine::GetResourceManager().GetResource<Engine::Resources::Texture>("TestTexture").lock());
        resources.push_back(Engine::GetResourceManager().GetResource<Engine::Resources::NormalMap>("TestNormalMap").lock());

        const auto sphere_model = Engine::Resources::Model::Create("SphereModel", resources);

        GetResourceManager().AddResource(sphere_model);
    }

    void fnClient()
    {
        InitializeTexture();
        InitializeMesh();
        InitializeNormal();
        InitializeModel();
        InitializeFont();
        InitializeSound();

        Engine::GetSceneManager().AddScene<Scene::TestScene>();
        Engine::GetSceneManager().SetActive<Scene::TestScene>();
    }
} // namespace Client
