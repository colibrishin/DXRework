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
	class Giftbox : public Engine::Abstract::Object
	{
	public:
		explicit Giftbox(const Engine::WeakScene& scene, const Engine::eLayerType layer);
		void Initialize_INTERNAL() override;
		~Giftbox() override;

		inline void PreUpdate(const float& dt) override;
		inline void Update(const float& dt) override;
		inline void PreRender(const float dt) override;
		inline void Render(const float dt) override;
	};

	inline Giftbox::Giftbox(const Engine::WeakScene& scene, const Engine::eLayerType layer) : Engine::Abstract::Object(scene, layer)
	{
	}

	inline void Giftbox::Initialize_INTERNAL()
	{
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::Mesh>(L"Giftbox"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::IShader>(L"vs_default"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::IShader>(L"ps_metalic"));

		AddComponent<Engine::Component::Transform>();
		const auto tr = GetComponent<Engine::Component::Transform>().lock();
		tr->SetPosition(Vector3(0.0f, -1.0f, 10.0f));
		tr->SetScale(Vector3::One);

		AddComponent<Engine::Component::Collider>();
		const auto cldr = GetComponent<Engine::Component::Collider>().lock();
		cldr->SetType(Engine::BOUNDING_TYPE_BOX);
		cldr->SetDirtyWithTransform(true);

		AddComponent<Engine::Component::Rigidbody>();
		const auto rb = GetComponent<Engine::Component::Rigidbody>().lock();
	}

	inline Giftbox::~Giftbox()
	{
	}

	inline void Giftbox::PreUpdate(const float& dt)
	{
		Object::PreUpdate(dt);
	}

	inline void Giftbox::Update(const float& dt)
	{
		Object::Update(dt);
		static float angle = 0.0f;

		const auto tr = GetComponent<Engine::Component::Transform>().lock();
		tr->SetRotation(Quaternion::CreateFromYawPitchRoll(angle, 0.0f, 0.0f));

		angle += dt;

		if(angle > XMConvertToRadians(360.0f))
		{
			angle = 0.0f;
		}
	}

	inline void Giftbox::PreRender(const float dt)
	{
		Object::PreRender(dt);
	}

	inline void Giftbox::Render(const float dt)
	{
		Object::Render(dt);
	}
}
