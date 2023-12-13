#include "pch.h"
#include "clTestCube.hpp"

#include "egCubeMesh.hpp"

CLIENT_OBJECT_IMPL(Client::Object::TestCube)

namespace Client::Object
{
	inline TestCube::TestCube() : Engine::Abstract::Object()
	{
	}

	inline void TestCube::Initialize()
	{
		AddResource(Engine::GetResourceManager().GetResource<Engine::Mesh::CubeMesh>("CubeMesh").lock());
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::Texture>("TestTexture").lock());
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::NormalMap>("TestNormalMap").lock());
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::VertexShader>("vs_default").lock());
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::PixelShader>("ps_normalmap_specular").lock());

		AddComponent<Engine::Component::Transform>();
		const auto tr = GetComponent<Engine::Component::Transform>().lock();
		tr->SetPosition(Vector3(2.0f, 4.0f, 0.0f));
		tr->SetScale(Vector3::One);

		AddComponent<Engine::Component::Collider>(GetResource<Engine::Resources::Mesh>("CubeMesh"));
		const auto cldr = GetComponent<Engine::Component::Collider>().lock();
		cldr->SetType(Engine::BOUNDING_TYPE_BOX);
		cldr->SetDirtyWithTransform(true);
		cldr->SetMass(1.0f);

		AddComponent<Engine::Component::Rigidbody>();
		const auto rb = GetComponent<Engine::Component::Rigidbody>().lock();

		rb->SetFrictionCoefficient(0.1f);
		rb->SetGravityOverride(true);

		AddComponent<Client::State::CharacterController>();
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
	}

	inline void TestCube::PreRender(const float& dt)
	{
		Object::PreRender(dt);
	}

	inline void TestCube::Render(const float& dt)
	{
		Object::Render(dt);
	}

	void TestCube::PostRender(const float& dt)
	{
		Object::PostRender(dt);
	}

	inline void TestCube::FixedUpdate(const float& dt)
	{
		Object::FixedUpdate(dt);
	}
}
