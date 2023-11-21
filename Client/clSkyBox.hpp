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
	class SkyBox : public Engine::Abstract::Object
	{
	public:
		SkyBox();
		void Initialize() override;
		~SkyBox() override;

		inline void PreUpdate(const float& dt) override;
		inline void Update(const float& dt) override;
		inline void PreRender(const float dt) override;
		inline void Render(const float dt) override;
	};

	inline SkyBox::SkyBox()
	{
	}

	inline void SkyBox::Initialize()
	{
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::Mesh>(L"BackSphereMesh"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::Texture>(L"Sky"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::IShader>(L"vs_default"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::IShader>(L"ps_default_nolight"));

		AddComponent<Engine::Component::Transform>();
		const auto tr = GetComponent<Engine::Component::Transform>().lock();
		tr->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
		tr->SetScale(Vector3::One * 15.0f);
		SetLayer(Engine::LAYER_DEFAULT);
	}

	inline SkyBox::~SkyBox()
	{
	}

	inline void SkyBox::PreUpdate(const float& dt)
	{
		Object::PreUpdate(dt);
	}

	inline void SkyBox::Update(const float& dt)
	{
		Object::Update(dt);
	}

	inline void SkyBox::PreRender(const float dt)
	{
		Object::PreRender(dt);
	}

	inline void SkyBox::Render(const float dt)
	{
		Object::Render(dt);
	}
}
