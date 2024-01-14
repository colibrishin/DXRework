#include "pch.h"
#include "clTestScene.hpp"

#include "clWater.hpp"
#include "egHelper.hpp"
#include <egCamera.h>

#include "clPlayer.h"
#include "clRifile.h"
#include "egApplication.h"
#include "egCollisionDetector.h"
#include "egTransform.h"

SERIALIZER_ACCESS_IMPL(
                       Client::Scene::TestScene,
                       _ARTAG(_BSTSUPER(Engine::Scene)))

namespace Client::Scene
{
    inline TestScene::TestScene()
    : Scene(SCENE_T_TEST),
      m_player_id_(g_invalid_id) {}

    inline void TestScene::PreUpdate(const float& dt)
    {
        Scene::PreUpdate(dt);
    }

    inline void TestScene::Update(const float& dt)
    {
        Scene::Update(dt);

        if (GetApplication().GetKeyState().Space)
        {
            RemoveGameObject(m_player_id_, Engine::LAYER_DEFAULT);
        }
    }

    inline void TestScene::PreRender(const float& dt)
    {
        Scene::PreRender(dt);
    }

    inline void TestScene::Render(const float& dt)
    {
        Scene::Render(dt);
    }

    void TestScene::PostRender(const float& dt)
    {
        Scene::PostRender(dt);
    }

    void TestScene::AddCustomObject()
    {
        if (ImGui::MenuItem("PlaneObject"))
        {
            CreateGameObject<Object::PlaneObject>(Engine::LAYER_DEFAULT);
        }
    }

    inline void TestScene::Initialize_INTERNAL()
    {
        const auto cube = CreateGameObject<Object::TestCube>(Engine::LAYER_DEFAULT).lock();
        cube->GetComponent<Engine::Components::Transform>().lock()->SetLocalPosition(
             {2.f, 4.f, 0.f});

        const auto sphere = CreateGameObject<Object::TestObject>(Engine::LAYER_DEFAULT).lock();
        sphere->GetComponent<Engine::Components::Transform>().lock()->SetLocalPosition(
            {-2.f, 4.f, 0.f});
        cube->AddChild(sphere);

        CreateGameObject<Object::FPSCounter>(Engine::LAYER_UI);
        CreateGameObject<Object::MousePositionText>(Engine::LAYER_UI);
        CreateGameObject<Object::SkyBox>(Engine::LAYER_SKYBOX);
        const auto ground = CreateGameObject<Object::PlaneObject>(Engine::LAYER_ENVIRONMENT).lock();

        const auto water = CreateGameObject<Object::Water>(Engine::LAYER_ENVIRONMENT).lock();
        water->GetComponent<Engine::Components::Transform>().lock()->SetLocalPosition(
            {0.f, 2.f, -2.f});

        const auto player = CreateGameObject<Object::Player>(Engine::LAYER_DEFAULT).lock();
        player->GetComponent<Engine::Components::Transform>().lock()->SetLocalPosition(
                   {-4.f, 2.f, 0.f});

        Engine::GetCollisionDetector().SetCollisionLayer(
                                                         Engine::LAYER_DEFAULT,
                                                         Engine::LAYER_ENVIRONMENT);

        m_player_id_ = player->GetID();
    }
} // namespace Client::Scene
