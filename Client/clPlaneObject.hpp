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
	class PlaneObject : public Engine::Abstract::Object
	{
	public:
		PlaneObject();
		void Initialize() override;
		~PlaneObject() override;

		inline void PreUpdate(const float& dt) override;
		inline void Update(const float& dt) override;
		inline void PreRender(const float dt) override;
		inline void Render(const float dt) override;
	};

	inline PlaneObject::PlaneObject()
	{
	}

	inline void PlaneObject::Initialize()
	{
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::Mesh>(L"CubeMesh"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::Texture>(L"TestTexture"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::NormalMap>(L"TestNormalMap"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::IShader>(L"vs_default"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::IShader>(L"ps_normalmap_metalic"));

		AddComponent<Engine::Component::Transform>();
		const auto tr = GetComponent<Engine::Component::Transform>().lock();
		tr->SetPosition(Vector3(0.0f, -1.0f, 0.0f));
		tr->SetScale({10.0f, 1.0f, 10.0f});

		AddComponent<Engine::Component::Collider>();
		const auto cldr = GetComponent<Engine::Component::Collider>().lock();
		cldr->SetType(Engine::BOUNDING_TYPE_BOX);
		cldr->SetDirtyWithTransform(true);
		cldr->SetMass(100000.0f);

		AddComponent<Engine::Component::Rigidbody>();
		const auto rb = GetComponent<Engine::Component::Rigidbody>().lock();

		rb->SetFixed(true);
		rb->SetFrictionCoefficient(0.2f);
		rb->SetGravityOverride(false);
	}

	inline PlaneObject::~PlaneObject()
	{
	}

	inline void PlaneObject::PreUpdate(const float& dt)
	{
		Object::PreUpdate(dt);
	}

	inline void PlaneObject::Update(const float& dt)
	{
		Object::Update(dt);
	}

	inline void PlaneObject::PreRender(const float dt)
	{
		Object::PreRender(dt);
	}

	inline void PlaneObject::Render(const float dt)
	{
		Object::Render(dt);
	}
}
