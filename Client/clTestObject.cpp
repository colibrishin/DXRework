#include "pch.h"
#include "clTestObject.hpp"

CLIENT_OBJECT_IMPL(Client::Object::TestObject)

namespace Client::Object
{
	inline void TestObject::Initialize()
	{
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::Mesh>(L"SphereMesh"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::Texture>(L"TestTexture"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::Texture>(L"TestNormalMap"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::IShader>(L"vs_default"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::IShader>(L"ps_normalmap"));

		AddComponent<Engine::Component::Transform>();
		const auto tr = GetComponent<Engine::Component::Transform>().lock();
		tr->SetPosition(Vector3(0.0f, 4.0f, 0.0f));
		tr->SetScale(Vector3::One);

		AddComponent<Engine::Component::Collider>();
		const auto cldr = GetComponent<Engine::Component::Collider>().lock();
		cldr->SetType(Engine::BOUNDING_TYPE_SPHERE);
		cldr->SetDirtyWithTransform(true);
		cldr->SetMass(1.0f);

		AddComponent<Engine::Component::Rigidbody>();
		const auto rb = GetComponent<Engine::Component::Rigidbody>().lock();
		rb->SetFrictionCoefficient(0.1f);
		rb->SetGravityOverride(true);
	}

	inline void TestObject::PreUpdate(const float& dt)
	{
		Object::PreUpdate(dt);		
	}

	inline void TestObject::Update(const float& dt)
	{
		Object::Update(dt);
	}

	inline void TestObject::PreRender(const float dt)
	{
		Object::PreRender(dt);
	}

	inline void TestObject::Render(const float dt)
	{
		Object::Render(dt);
	}
}
