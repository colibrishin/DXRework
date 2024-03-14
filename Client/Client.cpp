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

#include "clButtonScript.h"
#include "clHitboxScript.hpp"
#include "clHpTextScript.h"
#include "clTriangleMesh.hpp"
#include "clParticleCompute.h"
#include "clPlayerScript.h"
#include "clShadowIntersectionScript.h"
#include "egCollisionDetector.h"
#include "egComputeShader.h"
#include "egGlobal.h"
#include "egMaterial.h"
#include "egPointMesh.h"
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
    GetResourceManager().AddResource
      (
       "PointMesh", boost::make_shared<Engine::Meshes::PointMesh>()
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
    Resources::Shape::Create("BobShape", "./bob_lamp_update_export.fbx");
    //Resources::Shape::Create("CharacterShape", "./Character.fbx");
    Resources::Shape::Create("RifleShape", "./Rifle.fbx");
    Resources::Shape::Create("PlayerShape", "./player.obj");
    Resources::Shape::Create("MissileShape", "./Rocket.fbx");

    const auto cube = Resources::Shape::Create("CubeShape", "");
    cube->Add(Resources::Mesh::Get("CubeMesh"));

    const auto sphere = Resources::Shape::Create("SphereShape", "");
    sphere->Add(Resources::Mesh::Get("SphereMesh"));

    const auto point = Resources::Shape::Create("PointShape", "");
    point->Add(Resources::Mesh::Get("PointMesh"));
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
      mtr->SetResource<Resources::Shape>("CubeShape");

      // this is not necessary if user adds texture in order.
      //mtr->SetTextureSlot("TestTexture", 0);
      //mtr->SetTextureSlot("TestNormalMap", 1);
    }

    {
      const auto mtr = Resources::Material::Create("NormalSpecularSphere", "");
      mtr->SetResource<Resources::Texture>("TestTexture");
      mtr->SetResource<Resources::Texture>("TestNormalMap");
      mtr->SetResource<Resources::Shader>("specular_normal");
      mtr->SetResource<Resources::Shape>("SphereShape");
    }

    {
      const auto mtr = Resources::Material::Create("ColorCube", "");
      mtr->SetResource<Resources::Shader>("color");
      mtr->SetResource<Resources::Shape>("CubeShape");
    }

    {
      const auto mtr = Resources::Material::Create("ColorMissile", "");
      mtr->SetResource<Resources::Shader>("color");
      mtr->SetResource<Resources::Shape>("MissileShape");
    }

    /*{
      const auto mtr = Resources::Material::Create("Character", "");
      mtr->SetResource<Resources::Shader>("color");

      for (const auto& anim : Resources::Shape::Get("CharacterShape").lock()->GetAnimationCatalog())
      {
        mtr->SetResource<Resources::BoneAnimation>(anim);
      }

      mtr->SetResource<Resources::Shape>("CharacterShape");
    }*/

    {
      const auto mtr = Resources::Material::Create("ThunderSkybox", "");
      mtr->SetResource<Resources::Texture>("ThunderCat");
      mtr->SetResource<Resources::Shader>("skybox");
      mtr->SetResource<Resources::Shape>("SphereShape");
    }

    {
      const auto mtr = Resources::Material::Create("ColorRifle", "");
      mtr->SetResource<Resources::Shader>("color");
      mtr->SetResource<Resources::BaseAnimation>("FireAnimation");
      mtr->SetResource<Resources::Shape>("RifleShape");
    }

    {
      const auto mtr = Resources::Material::Create("BlueSkybox", "");
      mtr->SetResource<Resources::Texture>("Sky");
      mtr->SetResource<Resources::Shader>("skybox");
      mtr->SetResource<Resources::Shape>("SphereShape");
    }

    {
      const auto mtr = Resources::Material::Create("WaterCube", "");
      mtr->SetResource<Resources::Texture>("WaterNormalMap");
      mtr->SetResource<Resources::Shader>("refraction");
      mtr->SetResource<Resources::Shape>("CubeShape");
    }

    {
      const auto mtr = Resources::Material::Create("Billboard", "");
      mtr->SetResource<Resources::Texture>("TestTexture");
      mtr->SetResource<Resources::Shader>("billboard");
      mtr->SetResource<Resources::Shape>("PointShape");
    }
  }

  void Initialize(HWND hwnd)
  {
    Manager::Application::GetInstance().Initialize(hwnd);

    Script::Register<Scripts::HitboxScript>();
    Script::Register<Scripts::PlayerScript>();
    Script::Register<Scripts::HpTextScript>();
    Script::Register<Scripts::ShadowIntersectionScript>();
    Script::Register<Scripts::ButtonScript>();
    Resources::ComputeShader::Create<ComputeShaders::ParticleCompute>();

    // todo: refactor
    InitializeTexture();
    InitializeMesh();
    InitializeNormal();
    InitializeModel();
    InitializeAnimation();
    InitializeFont();
    InitializeSound();
    InitializeMaterial();

    GetCollisionDetector().UnsetCollisionLayer(LAYER_HITBOX, LAYER_HITBOX);
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
