#include "pch.h"
#include "clTestScene.hpp"

#include <egCamera.h>
#include "clWater.hpp"
#include "egHelper.hpp"

#include "clPlayer.h"
#include "clRifile.h"
#include "egApplication.h"
#include "egCollisionDetector.h"
#include "egTransform.h"

SERIALIZER_ACCESS_IMPL
(
 Client::Scene::TestScene,
 _ARTAG(_BSTSUPER(Engine::Scene))
)

namespace Client::Scene
{
  inline TestScene::TestScene()
    : Scene(SCENE_T_TEST),
      m_player_id_(g_invalid_id) {}

  inline void TestScene::PreUpdate(const float& dt) { Scene::PreUpdate(dt); }

  inline void TestScene::Update(const float& dt)
  {
    Scene::Update(dt);

    if (GetApplication().GetKeyState().Space) { RemoveGameObject(m_player_id_, LAYER_DEFAULT); }
  }

  inline void TestScene::PreRender(const float& dt) { Scene::PreRender(dt); }

  inline void TestScene::Render(const float& dt) { Scene::Render(dt); }

  void TestScene::PostRender(const float& dt) { Scene::PostRender(dt); }

  void TestScene::AddCustomObject()
  {
    if (ImGui::MenuItem("PlaneObject")) { CreateGameObject<Object::PlaneObject>(LAYER_DEFAULT); }
  }

  inline void TestScene::Initialize_INTERNAL()
  {
    const auto cube = CreateGameObject<Object::TestCube>(LAYER_DEFAULT).lock();
    cube->GetComponent<Components::Transform>().lock()->SetLocalPosition
      (
       {2.f, 4.f, 0.f}
      );

    const auto sphere = CreateGameObject<Object::TestObject>(LAYER_DEFAULT).lock();
    sphere->GetComponent<Components::Transform>().lock()->SetLocalPosition
      (
       {-2.f, 4.f, 0.f}
      );
    cube->AddChild(sphere);

    CreateGameObject<Object::FPSCounter>(LAYER_UI);
    CreateGameObject<Object::MousePositionText>(LAYER_UI);
    CreateGameObject<Object::SkyBox>(LAYER_SKYBOX);
    const auto ground = CreateGameObject<Object::PlaneObject>(LAYER_ENVIRONMENT).lock();

    const auto water = CreateGameObject<Object::Water>(LAYER_ENVIRONMENT).lock();
    water->GetComponent<Components::Transform>().lock()->SetLocalPosition
      (
       {0.f, 2.f, -2.f}
      );

    const auto player = CreateGameObject<Object::Player>(LAYER_DEFAULT).lock();
    player->GetComponent<Components::Transform>().lock()->SetLocalPosition
      (
       {-4.f, 2.f, 0.f}
      );

    GetCollisionDetector().SetCollisionLayer
      (
       LAYER_DEFAULT,
       LAYER_ENVIRONMENT
      );

    m_player_id_ = player->GetID();
  }
} // namespace Client::Scene
