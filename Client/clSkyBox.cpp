#include "pch.h"
#include "clSkyBox.hpp"

#include "clBackSphereMesh.hpp"

CLIENT_OBJECT_IMPL(Client::Object::SkyBox)

namespace Client::Object
{
	inline SkyBox::SkyBox() : Engine::Abstract::Object()
	{
	}

	inline void SkyBox::Initialize()
	{
		AddResource(Engine::GetResourceManager().GetResource<Client::Mesh::BackSphereMesh>("BackSphereMesh").lock());
		AddResource(Engine::GetResourceManager().GetResource<Engine::Resources::Texture>("Sky").lock());
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::VertexShader>("vs_default").lock());
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::PixelShader>("ps_default_nolight").lock());

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

	void SkyBox::PostRender(const float& dt)
	{
		Object::PostRender(dt);
	}

	inline void SkyBox::PreRender(const float& dt)
	{
		Object::PreRender(dt);
	}

	inline void Client::Object::SkyBox::Render(const float& dt)
	{
		Object::Render(dt);
	}
}
