#include "pch.h"
#include "clTestScene.hpp"

#include "clWater.hpp"
#include "egHelper.hpp"
#include <egCamera.h>

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
        const auto companion = Engine::Instantiate<Object::TestCube>();

        AddGameObject(
                      companion,
                      Engine::LAYER_DEFAULT);
        AddGameObject(
                      Engine::Instantiate<Object::TestObject>(),
                      Engine::LAYER_DEFAULT);
        AddGameObject(Engine::Instantiate<Object::FPSCounter>(), Engine::LAYER_UI);
        AddGameObject(
                      Engine::Instantiate<Object::MousePositionText>(),
                      Engine::LAYER_UI);
        AddGameObject(Engine::Instantiate<Object::SkyBox>(), Engine::LAYER_SKYBOX);
        AddGameObject(
                      Engine::Instantiate<Object::PlaneObject>(),
                      Engine::LAYER_ENVIRONMENT);

        const auto water = Engine::Instantiate<Object::Water>();

        water->GetComponent<Engine::Component::Transform>().lock()->SetPosition(
                                                                                {0.f, 0.f, -2.f});

        AddGameObject(water, Engine::LAYER_ENVIRONMENT);

        Engine::GetCollisionDetector().SetCollisionLayer(
                                                         Engine::LAYER_DEFAULT,
                                                         Engine::LAYER_ENVIRONMENT);

        GetMainCamera().lock()->BindObject(companion);
    }
} // namespace Client::Scene
