#pragma once
#include "clGiftboxObject.hpp"
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
		AddGameObject<Object::TestCube>(Engine::Instantiate<Object::TestCube>(), Engine::LAYER_DEFAULT);
		AddGameObject<Object::TestObject>(Engine::Instantiate<Object::TestObject>(), Engine::LAYER_DEFAULT);
		AddGameObject<Object::Giftbox>(Engine::Instantiate<Object::Giftbox>(), Engine::LAYER_DEFAULT);
	}
}
