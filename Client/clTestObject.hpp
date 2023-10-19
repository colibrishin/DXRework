#pragma once

#include "clTriangleMesh.hpp"
#include "../Engine/egManagerHelper.hpp"
#include "../Engine/egTexture.hpp"
#include "../Engine/egObject.hpp"
#include "../Engine/egResourceManager.hpp"
#include "../Engine/egTransform.hpp"
#include "../Engine/egIShader.hpp"

namespace Engine::Component
{
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
		AddComponent<Engine::Component::Transform>();
		const auto tr = GetComponent<Engine::Component::Transform>().lock();
		tr->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
		tr->SetScale(Vector3::One);

		AddResource(Engine::GetResourceManager()->GetResource<Engine::Resources::Mesh>(L"SphereMesh"));
		AddResource(Engine::GetResourceManager()->GetResource<Engine::Resources::Texture>(L"TestTexture"));
		AddResource(Engine::GetResourceManager()->GetResource<Engine::Graphic::IShader>(L"vs_default"));
		AddResource(Engine::GetResourceManager()->GetResource<Engine::Graphic::IShader>(L"ps_default"));
	}

	inline TestObject::~TestObject()
	{
	}

	inline void TestObject::PreUpdate()
	{
		Object::PreUpdate();
		static float angle = 0.0f;

		GetComponent<Engine::Component::Transform>().lock()->SetRotation(Quaternion::CreateFromYawPitchRoll(angle, 0.0f, 0.0f));

		angle += Engine::GetDeltaTime();

		if(angle > XMConvertToRadians(360.0f))
		{
			angle = 0.0f;
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
