#pragma once
#include "egD3Device.hpp"
#include "GeometricPrimitive.h"

namespace Engine::Graphic
{
	class ToolkitAPI
	{
	public:
		static void Initialize();

		static void FrameBegin();
		static void FrameEnd();

		static SpriteBatch* GetSpriteBatch() { return m_sprite_batch_.get(); }

	private:
		inline static std::unique_ptr<GeometricPrimitive> m_geometric_primitive_ = nullptr;
		inline static std::unique_ptr<SpriteBatch> m_sprite_batch_ = nullptr;

	};

	inline void ToolkitAPI::Initialize()
	{
		m_geometric_primitive_ = GeometricPrimitive::CreateTeapot(D3Device::s_context_.Get());
		GeometricPrimitive::SetDepthBufferMode(true);

		m_sprite_batch_ = std::make_unique<SpriteBatch>(D3Device::s_context_.Get());
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
