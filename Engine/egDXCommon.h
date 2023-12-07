#pragma once
#include <SimpleMath.h>

#include "egCommon.hpp"

namespace Engine
{
	namespace Manager::Graphics
	{
		class RenderPipeline;
		class D3Device;
	}

	using namespace DirectX::SimpleMath;

	extern Manager::Graphics::D3Device& GetD3Device();
	extern Manager::Graphics::RenderPipeline& GetRenderPipeline();
	extern Manager::Graphics::ToolkitAPI& GetToolkitAPI();

	struct VertexElement
	{
		Vector3 position;
		Vector4 color;
		Vector2 texCoord;

		Vector3 normal;
		Vector3 tangent;
		Vector3 binormal;
	};

	struct PerspectiveBuffer
	{
		Matrix world;
		Matrix view;
		Matrix projection;
	};

	struct TransformBuffer
	{
		Matrix scale;
		Matrix rotation;
		Matrix translation;
	};

	struct LightPositionBuffer
	{
		// due to padding, type is vector4 instead of vector3
		Vector4 position[g_max_lights];
	};

	struct LightColorBuffer
	{
		Color color[g_max_lights];
	};

	struct SpecularBuffer
	{
		float specular_power;
		float padding[3];
		Color specular_color;
	};
}
