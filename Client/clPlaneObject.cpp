#include "pch.h"
#include "clPlaneObject.hpp"
#include "egSound.hpp"
#include "egObject.hpp"

CLIENT_OBJECT_IMPL(Client::Object::PlaneObject)

namespace Client::Object
{
	inline PlaneObject::PlaneObject()
	{
	}

	inline void PlaneObject::Initialize()
	{
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::Mesh>("CubeMesh"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::Texture>("TestTexture"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::NormalMap>("TestNormalMap"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::IShader>("vs_default"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::IShader>("ps_normalmap_metalic"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::Sound>("AmbientSound"));

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

		const auto test = GetResource<Engine::Resources::Sound>("AmbientSound");
		test.lock()->PlayLoop(GetSharedPtr<Object>());
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
