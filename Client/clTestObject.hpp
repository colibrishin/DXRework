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
	class TestObject : public Engine::Abstract::Object
	{
	public:
		TestObject();
		void Initialize() override;
		~TestObject() override;

		inline void PreUpdate() override;
		inline void Update() override;
		inline void PreRender() override;
		inline void Render() override;
	};

	inline TestObject::TestObject()
	{
	}

	inline void TestObject::Initialize()
	{
		AddResource(Engine::GetResourceManager()->GetResource<Engine::Resources::Mesh>(L"SphereMesh"));
		AddResource(Engine::GetResourceManager()->GetResource<Engine::Resources::Texture>(L"TestTexture"));
		AddResource(Engine::GetResourceManager()->GetResource<Engine::Resources::Texture>(L"TestNormalMap"));
		AddResource(Engine::GetResourceManager()->GetResource<Engine::Graphic::IShader>(L"vs_default"));
		AddResource(Engine::GetResourceManager()->GetResource<Engine::Graphic::IShader>(L"ps_normalmap"));

		AddComponent<Engine::Component::Transform>();
		const auto tr = GetComponent<Engine::Component::Transform>().lock();
		tr->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
		tr->SetScale(Vector3::One);

		AddComponent<Engine::Component::Collider>();
		const auto cldr = GetComponent<Engine::Component::Collider>().lock();
		cldr->SetType(Engine::BOUNDING_TYPE_SPHERE);
		cldr->SetDirtyWithTransform(true);

		AddComponent<Engine::Component::Rigidbody>();
		const auto rb = GetComponent<Engine::Component::Rigidbody>().lock();
		rb->SetVelocity({3.f, 0.f, 0.f});
	}

	inline TestObject::~TestObject()
	{
	}

	inline void TestObject::PreUpdate()
	{
		Object::PreUpdate();
		static float angle = 0.0f;

		const auto tr = GetComponent<Engine::Component::Transform>().lock();
		tr->SetRotation(Quaternion::CreateFromYawPitchRoll(angle, 0.0f, 0.0f));

		angle += Engine::GetDeltaTime();

		if(angle > XMConvertToRadians(360.0f))
		{
			angle = 0.0f;
		}

		const auto rb = GetComponent<Engine::Component::Rigidbody>().lock();
		const auto position = tr->GetPosition();

		if (position.x > 1.99f && position.x > 2.0f)
		{
			rb->SetVelocity({-3.f, 0.f, 0.f});
		}
		else if (position.x < -1.99f && position.x < -2.0f)
		{
			rb->SetVelocity({3.f, 0.f, 0.f});
		}
		
	}

	inline void TestObject::Update()
	{
		Object::Update();
	}

	inline void TestObject::PreRender()
	{
		Object::PreRender();
	}

	inline void TestObject::Render()
	{
		Object::Render();
	}
}
