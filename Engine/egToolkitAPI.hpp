#pragma once
#include <SpriteBatch.h>
#include <CommonStates.h>

#include "egCommon.hpp"
#include "egDXCommon.h"
#include "egD3Device.hpp"
#include "GeometricPrimitive.h"

namespace Engine::Manager::Graphics
{
	class ToolkitAPI final : public Abstract::Singleton<ToolkitAPI>
	{
	public:
		ToolkitAPI(SINGLETON_LOCK_TOKEN) : Singleton() {}
		void Initialize() override;

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void FixedUpdate(const float& dt) override;

		SpriteBatch* GetSpriteBatch() const { return m_sprite_batch_.get(); }
		CommonStates* GetCommonStates() const { return m_states_.get(); }

	private:
		void FrameBegin();
		void FrameEnd();

	private:
		std::unique_ptr<CommonStates> m_states_ = nullptr;
		std::unique_ptr<GeometricPrimitive> m_geometric_primitive_ = nullptr;
		std::unique_ptr<SpriteBatch> m_sprite_batch_ = nullptr;

	};

	inline void ToolkitAPI::Initialize()
	{
		m_states_ = std::make_unique<CommonStates>(GetD3Device().GetDevice());
		m_geometric_primitive_ = GeometricPrimitive::CreateTeapot(GetD3Device().GetContext());
		GeometricPrimitive::SetDepthBufferMode(true);

		m_sprite_batch_ = std::make_unique<SpriteBatch>(GetD3Device().GetContext());
	}

	inline void ToolkitAPI::PreUpdate(const float& dt)
	{
	}

	inline void ToolkitAPI::Update(const float& dt)
	{
	}

	inline void ToolkitAPI::PreRender(const float& dt)
	{
		FrameBegin();
	}

	inline void ToolkitAPI::Render(const float& dt)
	{
		FrameEnd();
	}

	inline void ToolkitAPI::FixedUpdate(const float& dt)
	{
	}

	inline void ToolkitAPI::FrameBegin()
	{
		m_sprite_batch_->Begin();
	}

	inline void ToolkitAPI::FrameEnd()
	{
		m_sprite_batch_->End();
	}
}
