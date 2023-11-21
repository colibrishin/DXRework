#pragma once

#include "clTriangleMesh.hpp"
#include "clVelocityCounter.hpp"
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
		TestObject() = default;
		void Initialize() override;
		~TestObject() override = default;

		inline void PreUpdate(const float& dt) override;
		inline void Update(const float& dt) override;
		inline void PreRender(const float dt) override;
		inline void Render(const float dt) override;

	private:
		std::shared_ptr<VelocityCounter> m_velocity_counter_;

	};

	inline void TestObject::Initialize()
	{
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::Mesh>(L"SphereMesh"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::Texture>(L"TestTexture"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::Texture>(L"TestNormalMap"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::IShader>(L"vs_default"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::IShader>(L"ps_normalmap"));

		m_velocity_counter_ = Engine::Instantiate<VelocityCounter>();

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

		SetLayer(Engine::LAYER_DEFAULT);
	}

	inline void TestObject::PreUpdate(const float& dt)
	{
		Object::PreUpdate(dt);		
	}

	inline void TestObject::Update(const float& dt)
	{
		Object::Update(dt);
		m_velocity_counter_->SetVelocity(GetComponent<Engine::Component::Rigidbody>().lock()->GetLinearMomentum());
		m_velocity_counter_->Update(dt);
	}

	inline void TestObject::PreRender(const float dt)
	{
		Object::PreRender(dt);
	}

	inline void TestObject::Render(const float dt)
	{
		Object::Render(dt);
		m_velocity_counter_->Render(dt);
	}
}
