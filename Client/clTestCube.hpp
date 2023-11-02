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

		AddComponent<Engine::Component::Rigidbody>();
		const auto rb = GetComponent<Engine::Component::Rigidbody>().lock();
		rb->SetVelocity({0.f, 0.f, 0.f});
		rb->SetMass(1.0f);
		rb->SetGravityOverride(true);
		rb->SetElastic(true);
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
		const auto vel = rb->GetVelocity();

		if (Engine::GetApplication().GetKeyState().W)
		{
			rb->SetVelocity({vel.x, vel.y, 0.01f});
		}
		if (Engine::GetApplication().GetKeyState().A)
		{
			rb->SetVelocity({-0.01f, vel.y, vel.z });
		}
		if (Engine::GetApplication().GetKeyState().S)
		{
			rb->SetVelocity({vel.x, vel.y, -0.01f});
		}
		if (Engine::GetApplication().GetKeyState().D)
		{
			rb->SetVelocity({0.01f, vel.y, vel.z });
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
