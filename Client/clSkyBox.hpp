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

		inline void PreUpdate() override;
		inline void Update() override;
		inline void PreRender() override;
		inline void Render() override;
	};

	inline SkyBox::SkyBox()
	{
	}

	inline void SkyBox::Initialize()
	{
		AddResource(Engine::GetResourceManager()->GetResource<Engine::Resources::Mesh>(L"BackSphereMesh"));
		AddResource(Engine::GetResourceManager()->GetResource<Engine::Resources::Texture>(L"Sky"));
		AddResource(Engine::GetResourceManager()->GetResource<Engine::Graphic::IShader>(L"vs_default"));
		AddResource(Engine::GetResourceManager()->GetResource<Engine::Graphic::IShader>(L"ps_default_nolight"));

		AddComponent<Engine::Component::Transform>();
		const auto tr = GetComponent<Engine::Component::Transform>().lock();
		tr->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
		tr->SetScale(Vector3::One * 15.0f);
	}

	inline SkyBox::~SkyBox()
	{
	}

	inline void SkyBox::PreUpdate()
	{
		Object::PreUpdate();
	}

	inline void SkyBox::Update()
	{
		Object::Update();
	}

	inline void SkyBox::PreRender()
	{
		Object::PreRender();
	}

	inline void SkyBox::Render()
	{
		Object::Render();
	}
}
