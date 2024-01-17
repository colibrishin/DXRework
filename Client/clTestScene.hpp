#pragma once
#include "Client.h"
#include "clFPSCounter.hpp"
#include "clMousePosition.hpp"
#include "clPlaneObject.hpp"
#include "clSkyBox.hpp"
#include "clTestCube.hpp"
#include "clTestObject.hpp"
#include "../Engine/egScene.hpp"

namespace Client::Scene
{
    class TestScene : public Engine::Scene
    {
    public:
        CLIENT_SCENE_T(Engine::SCENE_T_TEST)

        TestScene();
        ~TestScene() override = default;

        void Initialize_INTERNAL() override;
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;

    private:
        SERIALIZER_ACCESS
        GlobalEntityID m_player_id_;

        void AddCustomObject() override;
    };
} // namespace Client::Scene

BOOST_CLASS_EXPORT_KEY(Client::Scene::TestScene)
