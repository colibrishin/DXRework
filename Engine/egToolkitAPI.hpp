#pragma once
#include "egD3Device.hpp"
#include "GeometricPrimitive.h"

namespace Engine::Graphic
{
	class ToolkitAPI
	{
	public:
		static void Initialize();

	private:
		inline static std::unique_ptr<GeometricPrimitive> m_geometric_primitive_ = nullptr;

	};

	inline void ToolkitAPI::Initialize()
	{
		m_geometric_primitive_ = GeometricPrimitive::CreateTeapot(D3Device::s_context_.Get());
		GeometricPrimitive::SetDepthBufferMode(true);
	}
}
