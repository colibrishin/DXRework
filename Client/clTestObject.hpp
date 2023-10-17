#pragma once

#include "clTriangleMesh.hpp"
#include "../Engine/egMeshRenderer.hpp"
#include "../Engine/egObject.hpp"
#include "../Engine/egTransform.hpp"

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
		TestObject::Initialize();
	}

	inline void TestObject::Initialize()
	{
		AddComponent<Engine::Component::Transform>();
		const auto tr = GetComponent<Engine::Component::Transform>().lock();
		tr->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
		tr->SetScale(Vector3::One);

		AddComponent<Engine::Component::MeshRenderer>();
		const auto mr = GetComponent<Engine::Component::MeshRenderer>().lock();

		mr->SetMesh(std::make_shared<Client::Mesh::TriangleMesh>());
	}

	inline TestObject::~TestObject()
	{
	}

	inline void TestObject::PreUpdate()
	{
		Object::PreUpdate();
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
