#include "pch.h"
#include "clTestScene.hpp"

SERIALIZER_ACCESS_IMPL(Client::Scene::TestScene,
	_ARTAG(_BSTSUPER(Engine::Scene)))

namespace Client::Scene
{
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

		const auto companion = Engine::Instantiate<Object::TestCube>();
		AddGameObject(companion, Engine::LAYER_DEFAULT);
		AddGameObject(Engine::Instantiate<Object::TestObject>(), Engine::LAYER_DEFAULT);
		AddGameObject(Engine::Instantiate<Object::FPSCounter>(), Engine::LAYER_UI);
		AddGameObject(Engine::Instantiate<Object::MousePositionText>(), Engine::LAYER_UI);
		AddGameObject(Engine::Instantiate<Object::SkyBox>(), Engine::LAYER_DEFAULT);
		AddGameObject(Engine::Instantiate<Object::PlaneObject>(), Engine::LAYER_DEFAULT);

		GetMainCamera().lock()->BindObject(companion);
	}
}