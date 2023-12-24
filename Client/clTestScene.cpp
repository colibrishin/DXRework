#include "pch.h"
#include "clTestScene.hpp"

#include "clWater.hpp"
#include "egHelper.hpp"
#include <egCamera.h>

#include "clPlayer.h"

SERIALIZER_ACCESS_IMPL(
                       Client::Scene::TestScene,
                       _ARTAG(_BSTSUPER(Engine::Scene)))

namespace Client::Scene
{
    inline TestScene::TestScene() {}

    inline void TestScene::PreUpdate(const float& dt)
    {
        Scene::PreUpdate(dt);
    }

    inline void TestScene::Update(const float& dt)
    {
        Scene::Update(dt);
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
            const auto plane = Engine::Instantiate<Object::PlaneObject>();
            AddGameObject(plane, Engine::LAYER_DEFAULT);
        }
    }

    inline void TestScene::Initialize_INTERNAL()
    {
        const auto cube = Engine::Instantiate<Object::TestCube>();
        AddGameObject(cube, Engine::LAYER_DEFAULT);

        cube->GetComponent<Engine::Components::Transform>().lock()->SetLocalPosition(
             {2.f, 4.f, 0.f});

        const auto sphere = Engine::Instantiate<Object::TestObject>();
        AddGameObject(sphere, Engine::LAYER_DEFAULT);
        sphere->GetComponent<Engine::Components::Transform>().lock()->SetLocalPosition(
            {0.f, 4.f, 0.f});

        AddGameObject(
                      Engine::Instantiate<Object::FPSCounter>(),
                      Engine::LAYER_UI);
        AddGameObject(
                      Engine::Instantiate<Object::MousePositionText>(),
                      Engine::LAYER_UI);
        AddGameObject(
                      Engine::Instantiate<Object::SkyBox>(),
                      Engine::LAYER_SKYBOX);
        AddGameObject(
                      Engine::Instantiate<Object::PlaneObject>(),
                      Engine::LAYER_ENVIRONMENT);

        const auto water = Engine::Instantiate<Object::Water>();
        AddGameObject(water, Engine::LAYER_ENVIRONMENT);
        water->GetComponent<Engine::Components::Transform>().lock()->SetLocalPosition(
            {0.f, 0.f, -2.f});

        const auto player = Engine::Instantiate<Object::Player>();
        AddGameObject(player, Engine::LAYER_DEFAULT);
        player->GetComponent<Engine::Components::Transform>().lock()->SetLocalPosition(
                   {-4.f, 2.f, 0.f});

        GetMainCamera().lock()->BindObject(player);

        Engine::GetCollisionDetector().SetCollisionLayer(
                                                         Engine::LAYER_DEFAULT,
                                                         Engine::LAYER_ENVIRONMENT);
    }
} // namespace Client::Scene
