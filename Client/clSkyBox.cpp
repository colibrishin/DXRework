#include "pch.h"
#include "clSkyBox.hpp"

CLIENT_OBJECT_IMPL(Client::Object::SkyBox)

namespace Client::Object
{
	inline SkyBox::SkyBox() : Engine::Abstract::Object()
	{
	}

	inline void SkyBox::Initialize()
	{
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::Mesh>("BackSphereMesh"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::Texture>("Sky"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::IShader>("vs_default"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::IShader>("ps_default_nolight"));

		AddComponent<Engine::Component::Transform>();
		const auto tr = GetComponent<Engine::Component::Transform>().lock();
		tr->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
		tr->SetScale(Vector3::One * 15.0f);
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

	inline void Client::Object::SkyBox::Render(const float dt)
	{
		Object::Render(dt);
	}
}
