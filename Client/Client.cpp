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
#include "egTexture2D.h"

// TODO: This is an example of a library function
namespace Client
{
  using namespace Engine;

  void InitializeTexture()
  {
    Resources::Texture2D::Create("TestTexture", "./Texture.dds", {});
    Resources::Texture2D::Create("Sky", "./Sky.dds", {});
    Resources::Texture2D::Create("ThunderCat", "./ThunderCat.dds", {});
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
    Resources::Texture2D::Create("TestNormalMap", "./Texture-Normal.dds", {});
    Resources::Texture2D::Create("WaterNormalMap", "./Water-Normal.dds", {});
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
      const auto mtr = Resources::Material::Create("NormalLight", "");
      mtr->SetResource<Resources::Texture>("TestTexture");
      mtr->SetResource<Resources::Texture>("TestNormalMap");
      mtr->SetResource<Resources::Shader>("normal");

      // this is not necessary if user adds texture in order.
      //mtr->SetTextureSlot("TestTexture", 0);
      //mtr->SetTextureSlot("TestNormalMap", 1);
    }

    {
      const auto mtr = Resources::Material::Create("NormalLightSpecular", "");
      mtr->SetResource<Resources::Texture>("TestTexture");
      mtr->SetResource<Resources::Texture>("TestNormalMap");
      mtr->SetResource<Resources::Shader>("specular_normal");
    }

    {
      const auto mtr = Resources::Material::Create("ColorMaterial", "");
      mtr->SetResource<Resources::Shader>("color");
    }

    {
      const auto mtr = Resources::Material::Create("CharacterMaterial", "");
      mtr->SetResource<Resources::Shader>("color");

      for (const auto& anim : Resources::Shape::Get("CharacterModel").lock()->GetAnimationCatalog())
      {
        mtr->SetResource<Resources::BoneAnimation>(anim);
      }
    }

    {
      const auto mtr = Resources::Material::Create("ThunderSky", "");
      mtr->SetResource<Resources::Texture>("ThunderCat");
      mtr->SetResource<Resources::Shader>("skybox");
    }

    {
      const auto mtr = Resources::Material::Create("RifleColorMaterial", "");
      mtr->SetResource<Resources::Shader>("color");
      mtr->SetResource<Resources::BaseAnimation>("FireAnimation");
    }

    {
      const auto mtr = Resources::Material::Create("SkyboxMaterial", "");
      mtr->SetResource<Resources::Texture>("Sky");
      mtr->SetResource<Resources::Shader>("skybox");
    }

    {
      const auto mtr = Resources::Material::Create("WaterMaterial", "");
      mtr->SetResource<Resources::Texture>("WaterNormalMap");
      mtr->SetResource<Resources::Shader>("refraction");
    }
  }

  void Initialize(HWND hwnd)
  {
    Manager::Application::GetInstance().Initialize(hwnd);

    InitializeTexture();
    InitializeMesh();
    InitializeNormal();
    InitializeModel();
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
