#pragma once
#include "clFPSCounter.hpp"
#include "clGiftboxObject.hpp"
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
		TestScene();
		~TestScene() override = default;

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;

	private:

	};

	inline TestScene::TestScene()
	{
	}

	inline void TestScene::PreUpdate(const float& dt)
	{
		Scene::PreUpdate(dt);
	}

	inline void TestScene::Update(const float& dt)
	{
		Scene::Update(dt);
	}

	inline void TestScene::PreRender(const float dt)
	{
		Scene::PreRender(dt);
	}

	inline void TestScene::Render(const float dt)
	{
		Scene::Render(dt);
	}

	inline void TestScene::Initialize()
	{
		Scene::Initialize();

		const auto companion_id = AddGameObject(Engine::InstantiateObject<Object::TestCube>(GetSharedPtr<Scene>()), Engine::LAYER_DEFAULT);
		AddGameObject(Engine::InstantiateObject<Object::TestObject>(GetSharedPtr<Scene>()), Engine::LAYER_DEFAULT);
		//AddGameObject(Engine::InstantiateObject<Object::Giftbox>(GetSharedPtr<Scene>()), Engine::LAYER_DEFAULT);
		AddGameObject(Engine::InstantiateObject<Object::FPSCounter>(GetSharedPtr<Scene>()), Engine::LAYER_UI);
		AddGameObject(Engine::InstantiateObject<Object::MousePositionText>(GetSharedPtr<Scene>()), Engine::LAYER_UI);
		AddGameObject(Engine::InstantiateObject<Object::SkyBox>(GetSharedPtr<Scene>()), Engine::LAYER_DEFAULT);
		AddGameObject(Engine::InstantiateObject<Object::PlaneObject>(GetSharedPtr<Scene>()), Engine::LAYER_DEFAULT);

		GetMainCamera().lock()->BindObject(FindGameObject(companion_id));
	}
}
