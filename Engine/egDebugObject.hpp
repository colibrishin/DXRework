#pragma once
#include "egRenderPipeline.hpp"
#include "egResourceManager.hpp"
#include "egObject.hpp"

namespace Engine::Object
{
	class DebugObject : public Abstract::Object
	{
	public:
		DebugObject() : Object() {}
		~DebugObject() override;

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void FixedUpdate(const float& dt) override;

		void SetCollided(bool collided) { m_bCollided = collided; }

	private:
		bool m_bCollided = false;

	};

	inline void DebugObject::Initialize()
	{
		AddComponent<Component::Transform>();
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::IShader>(L"vs_default"));
		AddResource(Engine::GetResourceManager().GetResource<Engine::Graphic::IShader>(L"ps_default_nolight"));
	}

	inline DebugObject::~DebugObject()
	{
	}

	inline void DebugObject::PreUpdate(const float& dt)
	{
		Object::PreUpdate(dt);
	}

	inline void DebugObject::Update(const float& dt)
	{
		Object::Update(dt);
	}

	inline void DebugObject::PreRender(const float dt)
	{
		Object::PreRender(dt);
	}

	inline void DebugObject::Render(const float dt)
	{
		GetRenderPipeline().SetWireframeState();
		Object::Render(dt);
		GetRenderPipeline().SetFillState();
	}

	inline void DebugObject::FixedUpdate(const float& dt)
	{
		Object::FixedUpdate(dt);
	}
}
