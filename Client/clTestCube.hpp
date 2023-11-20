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
#include "../Engine/egNormalMap.hpp"

namespace Engine::Resources
{
	class NormalMap;
}

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

		inline void PreUpdate(const float& dt) override;
		inline void Update(const float& dt) override;
		inline void PreRender(const float dt) override;
		inline void Render(const float dt) override;
	};

	inline TestCube::TestCube()
	{
	}

	inline void TestCube::Initialize()
	{
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::Mesh>(L"CubeMesh"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::Texture>(L"TestTexture"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::NormalMap>(L"TestNormalMap"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::IShader>(L"vs_default"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::IShader>(L"ps_normalmap_metalic"));

		AddComponent<Engine::Component::Transform>();
		const auto tr = GetComponent<Engine::Component::Transform>().lock();
		tr->SetPosition(Vector3(2.0f, 4.0f, 0.0f));
		tr->SetScale(Vector3::One);

		AddComponent<Engine::Component::Collider>();
		const auto cldr = GetComponent<Engine::Component::Collider>().lock();
		cldr->SetType(Engine::BOUNDING_TYPE_BOX);
		cldr->SetDirtyWithTransform(true);
		cldr->SetMass(1.0f);

		AddComponent<Engine::Component::Rigidbody>();
		const auto rb = GetComponent<Engine::Component::Rigidbody>().lock();

		rb->SetFrictionCoefficient(0.1f);
		rb->SetGravityOverride(true);
	}

	inline TestCube::~TestCube()
	{
	}

	inline void TestCube::PreUpdate(const float& dt)
	{
		Object::PreUpdate(dt);
	}

	inline void TestCube::Update(const float& dt)
	{
		Object::Update(dt);

		const auto rb  = GetComponent<Engine::Component::Rigidbody>().lock();

		float speed = 1.0f;

		if (Engine::GetApplication().GetKeyState().IsKeyDown(Keyboard::W))
		{
			rb->AddForce({0, 0, speed});
		}

		if (Engine::GetApplication().GetKeyState().IsKeyDown(Keyboard::A))
		{
			rb->AddForce({-speed, 0, 0 });
		}

		if (Engine::GetApplication().GetKeyState().IsKeyDown(Keyboard::S))
		{
			rb->AddForce({0, 0, -speed});
		}

		if (Engine::GetApplication().GetKeyState().IsKeyDown(Keyboard::D))
		{
			rb->AddForce({speed, 0, 0 });
		}
	}

	inline void TestCube::PreRender(const float dt)
	{
		Object::PreRender(dt);
	}

	inline void TestCube::Render(const float dt)
	{
		Object::Render(dt);
	}
}
