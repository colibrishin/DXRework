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
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::Mesh>(L"CubeMesh"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::Texture>(L"TestTexture"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::NormalMap>(L"TestNormalMap"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::IShader>(L"vs_default"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::IShader>(L"ps_normalmap_metalic"));

		AddComponent<Engine::Component::Transform>();
		const auto tr = GetComponent<Engine::Component::Transform>().lock();
		tr->SetPosition(Vector3(0.0f, 20.0f, 0.0f));
		tr->SetScale(Vector3::One);

		AddComponent<Engine::Component::Collider>();
		const auto cldr = GetComponent<Engine::Component::Collider>().lock();
		cldr->SetType(Engine::BOUNDING_TYPE_BOX);
		cldr->SetDirtyWithTransform(true);
		cldr->SetMass(1.0f);

		AddComponent<Engine::Component::Rigidbody>();
		const auto rb = GetComponent<Engine::Component::Rigidbody>().lock();

		rb->SetElasticity(0.5f);
		rb->SetFrictionCoefficient(0.5f);
		rb->SetGravityOverride(true);
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

		const auto rb  = GetComponent<Engine::Component::Rigidbody>().lock();
		const auto accel = rb->GetAcceleration();

		float speed = 0.1f;

		if (Engine::GetApplication().GetKeyState().IsKeyDown(Keyboard::W))
		{
			rb->SetAcceleration({accel.x, accel.y, speed});
		}

		if (Engine::GetApplication().GetKeyState().IsKeyDown(Keyboard::A))
		{
			rb->SetAcceleration({-speed, accel.y, accel.z });
		}

		if (Engine::GetApplication().GetKeyState().IsKeyDown(Keyboard::S))
		{
			rb->SetAcceleration({accel.x, accel.y, -speed});
		}

		if (Engine::GetApplication().GetKeyState().IsKeyDown(Keyboard::D))
		{
			rb->SetAcceleration({speed, accel.y, accel.z });
		}

		static float angle = 0.0f;

		const auto tr = GetComponent<Engine::Component::Transform>().lock();
		tr->SetRotation(Quaternion::CreateFromYawPitchRoll(angle, 0.0f, 0.0f));

		angle += Engine::GetDeltaTime();

		if(angle > XMConvertToRadians(360.0f))
		{
			angle = 0.0f;
		}
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
