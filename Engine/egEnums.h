#pragma once
#include <boost/mpl/vector.hpp>
#include "egConstant.h"

namespace Engine::Resources
{
	class Prefab;
	class Mesh;
	class Shape;
	class Material;
	class ShadowTexture;
	class AnimationsTexture;
	class Shader;
	class Texture3D;
	class Texture2D;
	class Texture1D;
	class Sound;
	class ComputeShader;
	class Font;
	class BoneAnimation;
	class Bone;
	class BaseAnimation;
	class AtlasAnimationTexture;
	class AtlasAnimation;
}

namespace Engine
{
	enum eRaytracingSBType
	{
		SB_TYPE_RAYTRACING_TLAS,
		SB_TYPE_RAYTRACING_VERTEX,
		SB_TYPE_RAYTRACING_INDEX,
		SB_TYPE_RAYTRACING_END
	};

	enum eRaytracingTexSlot
	{
		RAYTRACING_TEX_SLOT_TEXTURE = 0,
		RAYTRACING_TEX_SLOT_NORMAL,
		RAYTRACING_TEX_SLOT_COUNT //todo: add more slots
	};

	enum eRaytracingCBType
	{
		RAYTRACING_CB_VIEWPORT = 0,
		RAYTRACING_CB_COUNT
	};

	enum eRaytracingCBLocalType
	{
		RAYTRACING_CB_LOCAL_MATERIAL = 0,
		RAYTRACING_CB_LOCAL_COUNT
	};

	enum eCommandList
	{
		COMMAND_LIST_PRE_RENDER,
		COMMAND_LIST_RENDER,
		COMMAND_LIST_POST_RENDER,
		COMMAND_LIST_UPDATE,
		COMMAND_LIST_COPY,
		COMMAND_LIST_COMPUTE,
		COMMAND_LIST_COUNT
	};

	enum eCommandTypes
	{
		COMMAND_TYPE_DIRECT = 0,
		COMMAND_TYPE_COPY,
		COMMAND_TYPE_COMPUTE,
		COMMAND_TYPE_COUNT
	};

	enum eCommandNativeType
	{
		COMMAND_NATIVE_DIRECT  = D3D12_COMMAND_LIST_TYPE_DIRECT,
		COMMAND_NATIVE_COPY    = D3D12_COMMAND_LIST_TYPE_COPY,
		COMMAND_NATIVE_COMPUTE = D3D12_COMMAND_LIST_TYPE_COMPUTE
	};

	enum eRasterizerSlot
	{
		RASTERIZER_SLOT_SRV,
		RASTERIZER_SLOT_CB,
		RASTERIZER_SLOT_UAV,
		RASTERIZER_SLOT_SAMPLER,
		RASTERIZER_SLOT_COUNT
	};

	enum eBindType
	{
		BIND_TYPE_SAMPLER = 0,
		BIND_TYPE_CB,
		BIND_TYPE_UAV,
		BIND_TYPE_SRV,
		BIND_TYPE_RTV,
		BIND_TYPE_DSV,
		BIND_TYPE_DSV_ONLY,
		BIND_TYPE_COUNT
	};

	enum eTexBindSlot
	{
		BIND_SLOT_TEX     = 0,
		BIND_SLOT_TEXARR  = BIND_SLOT_TEX + 4,
		BIND_SLOT_TEXCUBE = BIND_SLOT_TEXARR + 2,
		BIND_SLOT_TEX1D   = BIND_SLOT_TEXCUBE + 2,
		BIND_SLOT_END     = BIND_SLOT_TEX1D + 2,
	};

	enum eSBType
	{
		SB_TYPE_LIGHT = BIND_SLOT_END,
		SB_TYPE_LIGHT_VP,
		SB_TYPE_INSTANCE,
		SB_TYPE_LOCAL_PARAM,
		SB_TYPE_MATERIAL,
		SB_TYPE_END
	};

	enum eReservedTexBindSlot
	{
		RESERVED_TEX_RENDERED = SB_TYPE_END,
		RESERVED_TEX_BONES,
		RESERVED_TEX_ATLAS,
		RESERVED_TEX_SHADOW_MAP,
		RESERVED_TEX_END = RESERVED_TEX_SHADOW_MAP + g_max_lights,
	};

	enum eTexUAVBindSlot
	{
		BIND_SLOT_UAV_TEX_1D = 0,
		BIND_SLOT_UAV_TEX_2D = BIND_SLOT_UAV_TEX_1D + 2,
		BIND_SLOT_UAV_TEXARR = BIND_SLOT_UAV_TEX_2D + 2,
		BIND_SLOT_UAV_END,
	};

	enum eSBUAVType
	{
		SB_TYPE_UAV_INSTANCE = BIND_SLOT_UAV_END,
		SB_TYPE_UAV_RESERVED_1,
		SB_TYPE_UAV_RESERVED_2,
		SB_TYPE_UAV_END,
	};

	static_assert(BIND_SLOT_UAV_END < 8);

	enum eCBType
	{
		CB_TYPE_WVP = 0,
		CB_TYPE_PARAM,
		CB_TYPE_END,
	};

	enum eSampler
	{
		SAMPLER_TEXTURE = 0,
		SAMPLER_SHADOW,
		SAMPLER_END,
	};

	enum eToolkitRenderType
	{
		TOOLKIT_RENDER_UNKNOWN   = 0,
		TOOLKIT_RENDER_PRIMITIVE = 1,
		TOOLKIT_RENDER_SPRITE,
	};

	enum eClientSBType : UINT;
	enum eClientSBUAVType : UINT;

	constexpr UINT g_max_engine_texture_slots = RESERVED_TEX_END;
	constexpr UINT g_max_cb_slots             = CB_TYPE_END;
	constexpr UINT g_max_uav_slots            = SB_TYPE_UAV_END;
	constexpr UINT g_max_sampler_slots        = SAMPLER_END;
	constexpr UINT g_total_engine_slots       = g_max_engine_texture_slots + g_max_cb_slots + g_max_uav_slots;

	constexpr UINT g_srv_offset = 0;
	constexpr UINT g_cb_offset  = g_max_engine_texture_slots;
	constexpr UINT g_uav_offset = g_cb_offset + g_max_cb_slots;

	enum eObserverState
	{
		OBSERVER_STATE_NONE,
	};

	enum eTaskType
	{
		TASK_NONE = 0,
		TASK_TOGGLE_RASTER,

		TASK_CHANGE_LAYER,
		TASK_ADD_OBJ,
		TASK_ADD_CHILD,
		TASK_ADD_COMPONENT,
		TASK_ADD_SCRIPT,

		TASK_CACHE_COMPONENT,
		TASK_UNCACHE_COMPONENT,
		TASK_CACHE_SCRIPT,
		TASK_UNCACHE_SCRIPT,

		TASK_TF_UPDATE,

		TASK_REM_COMPONENT,
		TASK_REM_SCRIPT,
		TASK_REM_CHILD,
		TASK_REM_OBJ,

		TASK_SYNC_SCENE,
		TASK_INIT_SCENE,
		TASK_REM_SCENE,
		TASK_ACTIVE_SCENE,
		TASK_SCRIPT_EVENT,
		TASK_MAX
	};

	enum eLayerType
	{
		LAYER_NONE = 0,
		LAYER_LIGHT,
		LAYER_PLAYER,
		LAYER_DEFAULT,
		LAYER_HITBOX,
		LAYER_ENVIRONMENT,
		LAYER_SKYBOX,
		LAYER_UI,
		LAYER_CAMERA,
		LAYER_MAX,
	};

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

	enum eTexType
	{
		TEX_TYPE_1D,
		TEX_TYPE_2D,
		TEX_TYPE_3D
	};

	enum eShaderDomain : UINT
	{
		SHADER_DOMAIN_OPAQUE = 0,
		SHADER_DOMAIN_MASK,
		SHADER_DOMAIN_TRANSPARENT,
		SHADER_DOMAIN_POST_PROCESS,
		SHADER_DOMAIN_MAX,
	};

	enum eShaderDepth : UINT
	{
		SHADER_DEPTH_TEST_ZERO = 0,
		SHADER_DEPTH_TEST_ALL  = 1,

		SHADER_DEPTH_NEVER         = 2,
		SHADER_DEPTH_LESS          = 4,
		SHADER_DEPTH_EQUAL         = 8,
		SHADER_DEPTH_LESS_EQUAL    = 16,
		SHADER_DEPTH_GREATER       = 32,
		SHADER_DEPTH_NOT_EQUAL     = 64,
		SHADER_DEPTH_GREATER_EQUAL = 128,
		SHADER_DEPTH_ALWAYS        = 256,
	};

	enum eShaderSampler : UINT
	{
		SHADER_SAMPLER_CLAMP        = 0,
		SHADER_SAMPLER_WRAP         = 1,
		SHADER_SAMPLER_MIRROR       = 2,
		SHADER_SAMPLER_BORDER       = 4,
		SHADER_SAMPLER_MIRROR_ONCE  = 8,
		shader_sampler_address_mask = 15,

		SHADER_SAMPLER_NEVER         = 16,
		SHADER_SAMPLER_LESS          = 32,
		SHADER_SAMPLER_EQUAL         = 64,
		SHADER_SAMPLER_LESS_EQUAL    = 128,
		SHADER_SAMPLER_GREATER       = 256,
		SHADER_SAMPLER_NOT_EQUAL     = 512,
		SHADER_SAMPLER_GREATER_EQUAL = 1024,
		SHADER_SAMPLER_ALWAYS        = 2048,
	};

	enum eShaderRasterizer : UINT
	{
		SHADER_RASTERIZER_CULL_NONE      = 0,
		SHADER_RASTERIZER_CULL_FRONT     = 1,
		SHADER_RASTERIZER_CULL_BACK      = 2,
		SHADER_RASTERIZER_FILL_WIREFRAME = 4,
		SHADER_RASTERIZER_FILL_SOLID     = 8,
	};

	enum eResourceType
	{
		RES_T_UNK = 0,
		RES_T_SHADER,
		RES_T_TEX, // Broad definition of texture
		RES_T_FONT,
		RES_T_SOUND,
		RES_T_BONE_ANIM,
		RES_T_BONE,
		RES_T_BASE_ANIM,
		RES_T_MTR,
		RES_T_MESH,
		RES_T_SHAPE,
		RES_T_ANIMS_TEX,
		RES_T_COMPUTE_SHADER,
		RES_T_SHADOW_TEX,
		RES_T_PREFAB,
		RES_T_ATLAS_TEX,
		RES_T_ATLAS_ANIM,
		RES_T_MAX,
	};

	constexpr const char* g_resource_type_str[] =
	{
		"Unknown",
		"Shader",
		"Texture",
		"Font",
		"Sound",
		"Bone Animation",
		"Bone",
		"Base Animation",
		"Material",
		"Mesh",
		"Shape",
		"Animation Texture",
		"Compute Shader",
		"Shadow Texture",
		"Prefab",
		"Atlas Texture",
		"Atlas Animation"
	};

	using LoadableResourceTypes = boost::mpl::vector<
		Resources::ComputeShader,
		Resources::BaseAnimation,
		Resources::Bone,
		Resources::BoneAnimation,
		Resources::Font,
		Resources::Mesh,
		Resources::Shader,
		Resources::Sound,
		Resources::AnimationsTexture,
		Resources::ShadowTexture,
		Resources::Texture1D,
		Resources::Texture2D,
		Resources::Texture3D,
		Resources::Material,
		Resources::Shape,
		Resources::Prefab,
		Resources::AtlasAnimationTexture,
		Resources::AtlasAnimation>;

	static_assert(ARRAYSIZE(g_resource_type_str) == RES_T_MAX);

	enum eComponentType
	{
		COM_T_UNK = 0,
		COM_T_TRANSFORM,
		COM_T_COLLIDER,
		COM_T_RIDIGBODY,
		COM_T_STATE,
		COM_T_SOUND_PLAYER,
		COM_T_ANIMATOR,
		COM_T_RENDERER,
		COM_T_SCRIPT,
	};

	enum eRenderComponentType
	{
		RENDER_COM_T_UNK = 0,
		RENDER_COM_T_MODEL,
		RENDER_COM_T_PARTICLE,
	};

	enum eDefObjectType
	{
		DEF_OBJ_T_UNK = 0,
		DEF_OBJ_T_NONE,
		DEF_OBJ_T_CAMERA,
		DEF_OBJ_T_LIGHT,
		DEF_OBJ_T_OBSERVER,
		DEF_OBJ_T_TEXT,
	};

	enum eLightType
	{
		LIGHT_T_UNK = 0,
		LIGHT_T_DIRECTIONAL,
		LIGHT_T_SPOT,
	};

	enum eBoundingType
	{
		BOUNDING_TYPE_BOX = 0,
		BOUNDING_TYPE_SPHERE,
	};

	constexpr const char* g_layer_type_str[] =
	{
		"None",
		"Light",
		"Player",
		"Default",
		"Hitbox",
		"Environment",
		"Skybox",
		"UI",
		"Camera",
	};

	static_assert(ARRAYSIZE(g_layer_type_str) == LAYER_MAX);

	// THIS ENUM SHOULD BE DEFINED AT THE CLIENT!
	enum eSceneType : UINT;
	// THIS ENUM SHOULD BE DEFINED AT THE CLIENT!
	enum eScriptType : UINT;
}
