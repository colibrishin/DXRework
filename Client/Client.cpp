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
#include <egBaseAnimation.h>

#include "clBackSphereMesh.hpp"
#include "clTestScene.hpp"
#include "egGlobal.h"
#include "egMaterial.h"
#include "egShape.h"

// TODO: This is an example of a library function
namespace Client
{
    using namespace Engine;

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
        Resources::Shape::Create("BobModel", "./bob_lamp_update_export.fbx");
        Resources::Shape::Create("CharacterModel", "./Character.fbx");
        Resources::Shape::Create("RifleModel", "./Rifle.fbx");
        Resources::Shape::Create("PlayerModel", "./player.obj");

        const auto cube = Resources::Shape::Create("CubeModel", "");
        cube->Add(Resources::Mesh::Get("CubeMesh"));

        const auto sphere = Resources::Shape::Create("SphereModel", "");
        sphere->Add(Resources::Mesh::Get("SphereMesh"));

        const auto skybox = Resources::Shape::Create("SkyboxModel", "");
        skybox->Add(Resources::Mesh::Get("BackSphereMesh"));
    }

    void InitializeAnimation()
    {
        Graphics::BoneAnimationPrimitive primitive;

        primitive.AddPosition(0.0f, {-.1f, 1.5f, 0.f});
        primitive.AddPosition(0.5f, {-.1f, 1.5f, -.1f});
        primitive.AddPosition(1.0f, {-.1f, 1.5f, 0.f});

        primitive.AddRotation(0.0f, Quaternion::CreateFromYawPitchRoll(0.f, XMConvertToRadians(90.f), 0.f));
        primitive.AddRotation(0.5f, Quaternion::CreateFromYawPitchRoll(0.f, XMConvertToRadians(90.f), 0.f));
        primitive.AddRotation(1.0f, Quaternion::CreateFromYawPitchRoll(0.f, XMConvertToRadians(90.f), 0.f));

        primitive.AddScale(0.0f, {1.f, 1.f, 1.f});
        primitive.AddScale(1.0f, {1.f, 1.f, 1.f});

        const auto anim = Resources::BaseAnimation::Create("FireAnimation", primitive);
        anim->SetDuration(1.f);
        anim->SetTicksPerSecond(30.f);
    }

    void InitializeMaterial()
    {
        const auto mtr = Resources::Material::Create("NormalLight", "");
        mtr->SetResource<Resources::Texture>("TestTexture");
        mtr->SetResource<Resources::NormalMap>("TestNormalMap");
        mtr->SetResource<Resources::VertexShader>("vs_default");
        mtr->SetResource<Resources::PixelShader>("ps_normalmap");

        const auto mtr2 = Resources::Material::Create("NormalSpecular", "");
        mtr2->SetResource<Resources::Texture>("TestTexture");
        mtr2->SetResource<Resources::NormalMap>("TestNormalMap");
        mtr2->SetResource<Resources::VertexShader>("vs_default");
        mtr2->SetResource<Resources::PixelShader>("ps_normalmap_specular");

        const auto mtr3 = Resources::Material::Create("ColorMaterial", "");
        mtr3->SetResource<Resources::VertexShader>("vs_default");
        mtr3->SetResource<Resources::PixelShader>("ps_color");

        const auto mtr4 = Resources::Material::Create("SkyboxMaterial", "");
        mtr4->SetResource<Resources::Texture>("Sky");
        mtr4->SetResource<Resources::VertexShader>("vs_default");
        mtr4->SetResource<Resources::PixelShader>("ps_default_nolight");

        const auto mtr5 = Resources::Material::Create("WaterMaterial", "");
        mtr5->SetResource<Resources::NormalMap>("WaterNormalMap");
        mtr5->SetResource<Resources::VertexShader>("vs_default");
        mtr5->SetResource<Resources::PixelShader>("ps_refraction");

    }

    void Initialize(HWND hwnd)
    {
        Engine::Manager::Application::GetInstance().Initialize(hwnd);

        InitializeTexture();
        InitializeMesh();
        InitializeNormal();
        InitializeMaterial();
        InitializeModel();
        InitializeFont();
        InitializeSound();
        InitializeAnimation();

        Engine::GetSceneManager().AddScene<Scene::TestScene>();
        Engine::GetSceneManager().SetActive<Scene::TestScene>();
    }

    void Tick()
    {
        Engine::Manager::Application::GetInstance().Tick();
    }

    LRESULT MessageHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        return Engine::Manager::Application::GetInstance().MessageHandler(hwnd, msg, wparam, lparam);
    }

    UINT GetWidth()
    {
        return Engine::g_window_width;
    }

    UINT GetHeight()
    {
        return Engine::g_window_height;
    }

    bool IsFullScreen()
    {
        return Engine::g_full_screen;
    }
} // namespace Client
