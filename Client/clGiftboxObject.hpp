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
		Giftbox();
		void Initialize() override;
		~Giftbox() override;

		inline void PreUpdate() override;
		inline void Update() override;
		inline void PreRender() override;
		inline void Render() override;
	};

	inline Giftbox::Giftbox()
	{
	}

	inline void Giftbox::Initialize()
	{
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::Mesh>(L"Giftbox"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::IShader>(L"vs_default"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::IShader>(L"ps_metalic"));

		Engine::GetRenderPipeline().SetSpecularColor({0.5f, 0.5f, 0.5f, 1.0f});
		Engine::GetRenderPipeline().SetSpecularPower(100.0f);

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
		rb->SetVelocity({0.f, 0.f, 0.f});
	}

	inline Giftbox::~Giftbox()
	{
	}

	inline void Giftbox::PreUpdate()
	{
		Object::PreUpdate();
	}

	inline void Giftbox::Update()
	{
		Object::Update();
		static float angle = 0.0f;

		const auto tr = GetComponent<Engine::Component::Transform>().lock();
		tr->SetRotation(Quaternion::CreateFromYawPitchRoll(angle, 0.0f, 0.0f));

		angle += Engine::GetDeltaTime();

		if(angle > XMConvertToRadians(360.0f))
		{
			angle = 0.0f;
		}
	}

	inline void Giftbox::PreRender()
	{
		Object::PreRender();
	}

	inline void Giftbox::Render()
	{
		Object::Render();
	}
}
