// Client.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "Client.h"

#include <egBaseAnimation.h>
#include <egBoneAnimation.h>
#include <egCubeMesh.h>
#include <egFont.h>
#include <egResourceManager.hpp>
#include <egSceneManager.hpp>
#include <egSerialization.hpp>
#include <egSound.h>
#include <egSphereMesh.h>
#include <egSphereMesh.h>

#include "clDarkScene.h"
#include "clTestScene.hpp"
#include "egCollisionDetector.h"
#include "egGlobal.h"
#include "egMaterial.h"
#include "egShader.hpp"
#include "egShape.h"
#include "egTexture.h"

// TODO: This is an example of a library function
namespace Client
{
  using namespace Engine;

  void InitializeTexture()
  {
    Resources::Texture::Create("TestTexture", "./Texture.dds");
    Resources::Texture::Create("Sky", "./Sky.dds");
    Resources::Texture::Create("ThunderCat", "./ThunderCat.dds");
  }

  void InitializeMesh()
  {
    GetResourceManager().AddResource
      (
       "TriangleMesh", boost::make_shared<Meshes::TriangleMesh>()
      );
    GetResourceManager().AddResource
      (
       "CubeMesh", boost::make_shared<Engine::Meshes::CubeMesh>()
      );
    GetResourceManager().AddResource
      (
       "SphereMesh", boost::make_shared<Engine::Meshes::SphereMesh>()
      );
  }

  void InitializeNormal()
  {
    Resources::Texture::Create("TestNormalMap", "./Texture-Normal.dds");
    Resources::Texture::Create("WaterNormalMap", "./Water-Normal.dds");
  }

  void InitializeSound()
  {
    Resources::Sound::Create
      (
       "AmbientSound",
       "./crowded-avenue-people-talking-vendors-shouting-musicians-playing.mp3"
      );
  }

  void InitializeFont() { Resources::Font::Create("DefaultFont", "./consolas.spritefont"); }

  void InitializeShape()
  {
    Resources::Shape::Create("BobModel", "./bob_lamp_update_export.fbx");
    Resources::Shape::Create("CharacterModel", "./Character.fbx");
    Resources::Shape::Create("RifleModel", "./Rifle.fbx");
    Resources::Shape::Create("PlayerModel", "./player.obj");

    const auto cube = Resources::Shape::Create("CubeModel", "");
    cube->Add(Resources::Mesh::Get("CubeMesh"));

    const auto sphere = Resources::Shape::Create("SphereModel", "");
    sphere->Add(Resources::Mesh::Get("SphereMesh"));
  }

  void InitializeAnimation()
  {
    Graphics::BoneAnimationPrimitive primitive;

    primitive.AddPosition(0.0f, {-.1f, 1.5f, 0.f});
    primitive.AddPosition(0.5f, {-.1f, 1.5f, -.1f});
    primitive.AddPosition(1.0f, {-.1f, 1.5f, 0.f});

    primitive.AddRotation(0.0f, Quaternion::CreateFromYawPitchRoll(0.f, 0.f, 0.f));
    primitive.AddRotation(0.5f, Quaternion::CreateFromYawPitchRoll(0.f, 0.f, 0.f));
    primitive.AddRotation(1.0f, Quaternion::CreateFromYawPitchRoll(0.f, 0.f, 0.f));

    primitive.AddScale(0.0f, {1.f, 1.f, 1.f});
    primitive.AddScale(1.0f, {1.f, 1.f, 1.f});

    const auto anim = Resources::BaseAnimation::Create("FireAnimation", primitive);
    anim->SetDuration(1.f);
    anim->SetTicksPerSecond(30.f);
  }

  void InitializeMaterial()
  {
    {
      const auto mtr = Resources::Material::Create("NormalLightCube", "");
      mtr->SetResource<Resources::Texture>("TestTexture");
      mtr->SetResource<Resources::Texture>("TestNormalMap");
      mtr->SetResource<Resources::Shader>("normal");
      mtr->SetResource<Resources::Shape>("CubeModel");

      // this is not necessary if user adds texture in order.
      //mtr->SetTextureSlot("TestTexture", 0);
      //mtr->SetTextureSlot("TestNormalMap", 1);
    }

    {
      const auto mtr = Resources::Material::Create("NormalSpecularSphere", "");
      mtr->SetResource<Resources::Texture>("TestTexture");
      mtr->SetResource<Resources::Texture>("TestNormalMap");
      mtr->SetResource<Resources::Shader>("specular_normal");
      mtr->SetResource<Resources::Shape>("SphereModel");
    }

    {
      const auto mtr = Resources::Material::Create("ColorPlane", "");
      mtr->SetResource<Resources::Shader>("color");
      mtr->SetResource<Resources::Shape>("CubeMesh");
    }

    {
      const auto mtr = Resources::Material::Create("ColorCharacter", "");
      mtr->SetResource<Resources::Shader>("color");

      for (const auto& anim : Resources::Shape::Get("CharacterModel").lock()->GetAnimationCatalog())
      {
        mtr->SetResource<Resources::BoneAnimation>(anim);
      }

      mtr->SetResource<Resources::Shape>("CharacterModel");
    }

    {
      const auto mtr = Resources::Material::Create("ThunderSky", "");
      mtr->SetResource<Resources::Texture>("ThunderCat");
      mtr->SetResource<Resources::Shader>("skybox");
      mtr->SetResource<Resources::Shape>("SphereModel");
    }

    {
      const auto mtr = Resources::Material::Create("ColorRifle", "");
      mtr->SetResource<Resources::Shader>("color");
      mtr->SetResource<Resources::BaseAnimation>("FireAnimation");
      mtr->SetResource<Resources::Shape>("RifleModel");
    }

    {
      const auto mtr = Resources::Material::Create("BlueSky", "");
      mtr->SetResource<Resources::Texture>("Sky");
      mtr->SetResource<Resources::Shader>("skybox");
      mtr->SetResource<Resources::Shape>("SphereModel");
    }

    {
      const auto mtr = Resources::Material::Create("WaterCube", "");
      mtr->SetResource<Resources::Texture>("WaterNormalMap");
      mtr->SetResource<Resources::Shader>("refraction");
      mtr->SetResource<Resources::Shape>("CubeModel");
    }
  }

  void Initialize(HWND hwnd)
  {
    Manager::Application::GetInstance().Initialize(hwnd);

    InitializeTexture();
    InitializeMesh();
    InitializeNormal();
    InitializeShape();
    InitializeAnimation();
    InitializeFont();
    InitializeSound();
    InitializeMaterial();

    GetCollisionDetector().UnsetCollisionLayer(LAYER_HITBOX, LAYER_HITBOX);


    GetSceneManager().AddScene<Scene::TestScene>("Test");
    GetSceneManager().AddScene<Scene::DarkScene>("Thunder");
    GetSceneManager().SetActive<Scene::TestScene>("Test");
  }

  void Tick() { Manager::Application::GetInstance().Tick(); }

  LRESULT MessageHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
  {
    return Manager::Application::GetInstance().MessageHandler(hwnd, msg, wparam, lparam);
  }

  UINT GetWidth() { return g_window_width; }

  UINT GetHeight() { return g_window_height; }

  bool IsFullScreen() { return g_full_screen; }
} // namespace Client
