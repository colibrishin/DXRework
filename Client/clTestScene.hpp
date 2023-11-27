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
		const auto companion_id = AddGameObject(Engine::Instantiate<Object::TestCube>(), Engine::LAYER_DEFAULT);
		AddGameObject(Engine::Instantiate<Object::TestObject>(), Engine::LAYER_DEFAULT);
		//AddGameObject(Engine::Instantiate<Object::Giftbox>(), Engine::LAYER_DEFAULT);
		AddGameObject(Engine::Instantiate<Object::FPSCounter>(), Engine::LAYER_UI);
		AddGameObject(Engine::Instantiate<Object::MousePositionText>(), Engine::LAYER_UI);
		AddGameObject(Engine::Instantiate<Object::SkyBox>(), Engine::LAYER_DEFAULT);
		AddGameObject(Engine::Instantiate<Object::PlaneObject>(), Engine::LAYER_DEFAULT);

		//GetMainCamera().lock()->BindObject(FindGameObject(companion_id));
	}
}
