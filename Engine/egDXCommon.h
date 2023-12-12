#pragma once
#include <wrl/client.h>

#include <SimpleMath.h>

using namespace Microsoft::WRL;

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

	enum eShaderType
	{
		SHADER_VERTEX = 0,
		SHADER_PIXEL,
		SHADER_GEOMETRY,
		SHADER_COMPUTE,
		SHADER_HULL,
		SHADER_DOMAIN,
		SHADER_UNKNOWN
	};

	enum eCBType
	{
		CB_TYPE_WVP = 0,
		CB_TYPE_TRANSFORM,
		CB_TYPE_LIGHT,
		CB_TYPE_SPECULAR,
		CB_TYPE_SHADOW,
		CB_TYPE_SHADOW_CHUNK,
		CB_TYPE_WATER,
		CB_TYPE_CLIP_PLANE,
	};

	enum eShaderResource
	{
		SR_TEXTURE = 0,
		SR_NORMAL_MAP,
		SR_SHADOW_MAP,
		SR_REFLECT_MAP,
		SR_REFRACT_MAP,
	};

	enum eSampler
	{
		SAMPLER_TEXTURE = 0,
		SAMPLER_SHADOW,
	};

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
		Matrix reflectView;
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
		float ___p[3];
	};

	struct SpecularBuffer
	{
		float specular_power;
		float ___p[3];
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

	struct WaterBuffer
	{
		float water_translation;
		float reflect_refract_scale;
		float ___p[2];
	};

	struct ClipPlaneBuffer
	{
		Vector4 clip_plane;
	};
}
