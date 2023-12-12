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

	constexpr int g_max_lights = 8;
	constexpr int g_max_shadow_cascades = 3;
	constexpr int g_max_shadow_map_size = 512;

	struct GraphicShadowBuffer
	{
		ComPtr<ID3D11DepthStencilView> depth_stencil_view;
		ComPtr<ID3D11ShaderResourceView> shader_resource_view;
	};

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
		Matrix invView;
		Matrix invProj;
	};

	struct TransformBuffer
	{
		Matrix scale;
		Matrix rotation;
		Matrix translation;
	};

	struct LightBuffer
	{
		Matrix world[g_max_lights];
		Color color[g_max_lights];
		int light_count;
		float padding[3];
	};

	struct SpecularBuffer
	{
		float specular_power;
		float padding[3];
		Color specular_color;
	};

	struct CascadeShadow
	{
		Matrix view[g_max_shadow_cascades];
		Matrix proj[g_max_shadow_cascades];
		Vector4 end_clip_spaces[g_max_shadow_cascades];
	};

	struct CascadeShadowBuffer
	{
		CascadeShadow shadow;
	};

	struct CascadeShadowBufferChunk
	{
		CascadeShadow lights[g_max_lights];
	};
}
