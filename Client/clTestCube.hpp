#pragma once

#include "clTriangleMesh.hpp"
#include "../Engine/egCollider.hpp"
#include "../Engine/egManagerHelper.hpp"
#include "../Engine/egTexture.hpp"
#include "../Engine/egObject.hpp"
#include "../Engine/egResourceManager.hpp"
#include "../Engine/egTransform.hpp"
#include "../Engine/egIShader.hpp"
#include "../Engine/egRigidbody.hpp"

namespace Engine::Component
{
	class Rigidbody;
	class Transform;
}

namespace Client::Object
{
	class TestCube : public Engine::Abstract::Object
	{
	public:
		TestCube();
		void Initialize() override;
		~TestCube() override;

		inline void PreUpdate() override;
		inline void Update() override;
		inline void PreRender() override;
		inline void Render() override;
	};

	inline TestCube::TestCube()
	{
	}

	inline void TestCube::Initialize()
	{
		AddResource(Engine::GetResourceManager()->GetResource<Engine::Resources::Mesh>(L"CubeMesh"));
		AddResource(Engine::GetResourceManager()->GetResource<Engine::Resources::Texture>(L"TestTexture"));
		AddResource(Engine::GetResourceManager()->GetResource<Engine::Graphic::IShader>(L"vs_default"));
		AddResource(Engine::GetResourceManager()->GetResource<Engine::Graphic::IShader>(L"ps_default"));

		AddComponent<Engine::Component::Transform>();
		const auto tr = GetComponent<Engine::Component::Transform>().lock();
		tr->SetPosition(Vector3(0.0f, -1.0f, 0.0f));
		tr->SetScale(Vector3::One);

		AddComponent<Engine::Component::Collider>();
		const auto cldr = GetComponent<Engine::Component::Collider>().lock();
		cldr->SetType(Engine::BOUNDING_TYPE_BOX);
		cldr->SetDirtyWithTransform(true);

		AddComponent<Engine::Component::Rigidbody>();
		const auto rb = GetComponent<Engine::Component::Rigidbody>().lock();
		rb->SetVelocity({0.f, 0.f, 0.f});
		rb->SetFriction(0.25f);
	}

	inline TestCube::~TestCube()
	{
	}

	inline void TestCube::PreUpdate()
	{
		Object::PreUpdate();
	}

	inline void TestCube::Update()
	{
		Object::Update();
	}

	inline void TestCube::PreRender()
	{
		Object::PreRender();
	}

	inline void TestCube::Render()
	{
		Object::Render();
	}
}
