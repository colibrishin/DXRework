#include "pch.h"
#include "clDarkScene.h"

#include "clWater.hpp"
#include "egHelper.hpp"
#include <egCamera.h>

#include "clPlayer.h"
#include "clRifile.h"
#include "egApplication.h"
#include "egCollisionDetector.h"
#include "egMaterial.h"
#include "egModelRenderer.h"
#include "egTransform.h"
#include "egTexture.h"

SERIALIZER_ACCESS_IMPL(
                       Client::Scene::DarkScene,
                       _ARTAG(_BSTSUPER(Engine::Scene)))

namespace Client::Scene
{
    DarkScene::DarkScene() : Scene(SCENE_T_DARK) {}

    void DarkScene::PreUpdate(const float& dt)
    {
        Scene::PreUpdate(dt);
    }

    void DarkScene::Update(const float& dt)
    {
        Scene::Update(dt);
    }

    void DarkScene::PreRender(const float& dt)
    {
        Scene::PreRender(dt);
    }

    void DarkScene::Render(const float& dt)
    {
        Scene::Render(dt);
    }

    void DarkScene::PostRender(const float& dt)
    {
        Scene::PostRender(dt);
    }

    void DarkScene::Initialize_INTERNAL()
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
        const auto go = CreateGameObject<Object::SkyBox>(Engine::LAYER_SKYBOX).lock();

        go->GetComponent<Components::ModelRenderer>().lock()->SetMaterial(Resources::Material::Get("ThunderSky"));

        CreateGameObject<Object::PlaneObject>(Engine::LAYER_ENVIRONMENT);

        const auto water = CreateGameObject<Object::Water>(Engine::LAYER_ENVIRONMENT).lock();
        water->GetComponent<Engine::Components::Transform>().lock()->SetLocalPosition(
            {0.f, 0.f, -2.f});

        const auto player = CreateGameObject<Object::Player>(Engine::LAYER_DEFAULT).lock();
        player->GetComponent<Engine::Components::Transform>().lock()->SetLocalPosition(
                   {-4.f, 2.f, 0.f});

        player->AddChild(GetMainCamera());

        Engine::GetCollisionDetector().SetCollisionLayer(
                                                         Engine::LAYER_DEFAULT,
                                                         Engine::LAYER_ENVIRONMENT);

    }
} // namespace Client::Scene
