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
		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Render() override;

	private:

	};

	inline TestScene::TestScene()
	{
	}

	inline void TestScene::PreUpdate()
	{
		Scene::PreUpdate();
	}

	inline void TestScene::Update()
	{
		Scene::Update();
	}

	inline void TestScene::PreRender()
	{
		Scene::PreRender();
	}

	inline void TestScene::Render()
	{
		Scene::Render();
	}

	inline void TestScene::Initialize()
	{
		//AddGameObject(Engine::Instantiate<Object::TestCube>(), Engine::LAYER_DEFAULT);
		AddGameObject(Engine::Instantiate<Object::TestObject>(), Engine::LAYER_DEFAULT);
		//AddGameObject(Engine::Instantiate<Object::Giftbox>(), Engine::LAYER_DEFAULT);
		AddGameObject(Engine::Instantiate<Object::FPSCounter>(), Engine::LAYER_UI);
		AddGameObject(Engine::Instantiate<Object::MousePositionText>(), Engine::LAYER_UI);
		AddGameObject(Engine::Instantiate<Object::SkyBox>(), Engine::LAYER_DEFAULT);
		AddGameObject(Engine::Instantiate<Object::PlaneObject>(), Engine::LAYER_DEFAULT);
	}
}
