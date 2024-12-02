#pragma once
#include <boost/smart_ptr.hpp>
#include <string>
#include <filesystem>

#if defined(USE_DX12)
#include <directxtk12/SimpleMath.h>
#include <DirectXMath.h>

using Vector2 = DirectX::SimpleMath::Vector2;
using Vector3 = DirectX::SimpleMath::Vector3;
using Vector4 = DirectX::SimpleMath::Vector4;
using Color = DirectX::SimpleMath::Color;
using Quaternion = DirectX::SimpleMath::Quaternion;
using Ray = DirectX::SimpleMath::Ray;
using Matrix = DirectX::SimpleMath::Matrix;

namespace Microsoft::WRL
{
	template <typename T>
	class ComPtr;
}

namespace Engine 
{
	using DirectX::BoundingBox;
	using DirectX::BoundingFrustum;
	using DirectX::BoundingOrientedBox;
	using DirectX::BoundingSphere;

	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	using DirectX::XMFLOAT2;
	using DirectX::XMFLOAT3X3;
	using DirectX::XMVECTORF32;

	struct CommandPair;
	struct DescriptorHandler;
	struct DescriptorPtrImpl;

	using StrongDescriptorPtr = boost::shared_ptr<DescriptorPtrImpl>;
	using DescriptorPtr = boost::weak_ptr<DescriptorPtrImpl>;

	enum eBindType : uint8_t
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

	enum eTexBindSlot : uint8_t
	{
		BIND_SLOT_TEX = 0,
		BIND_SLOT_TEXARR = BIND_SLOT_TEX + 4,
		BIND_SLOT_TEXCUBE = BIND_SLOT_TEXARR + 2,
		BIND_SLOT_TEX1D = BIND_SLOT_TEXCUBE + 2,
		BIND_SLOT_END = BIND_SLOT_TEX1D + 2,
	};

	enum eSBType : uint8_t
	{
		SB_TYPE_LIGHT = BIND_SLOT_END,
		SB_TYPE_LIGHT_VP,
		SB_TYPE_INSTANCE,
		SB_TYPE_LOCAL_PARAM,
		SB_TYPE_MATERIAL,
		SB_TYPE_END
	};

	enum eRaytracingSBType
	{
		SB_TYPE_RAYTRACING_TLAS,
		SB_TYPE_RAYTRACING_VERTEX,
		SB_TYPE_RAYTRACING_INDEX,
		SB_TYPE_RAYTRACING_END
	};

	enum eReservedTexBindSlot : uint8_t
	{
		RESERVED_TEX_RENDERED = SB_TYPE_END,
		RESERVED_TEX_BONES,
		RESERVED_TEX_ATLAS,
		RESERVED_TEX_SHADOW_MAP,
		RESERVED_TEX_END = RESERVED_TEX_SHADOW_MAP + CFG_MAX_DIRECTIONAL_LIGHT,
	};

	enum eTexUAVBindSlot : uint8_t
	{
		BIND_SLOT_UAV_TEX_1D = 0,
		BIND_SLOT_UAV_TEX_2D = BIND_SLOT_UAV_TEX_1D + 2,
		BIND_SLOT_UAV_TEXARR = BIND_SLOT_UAV_TEX_2D + 2,
		BIND_SLOT_UAV_END,
	};

	enum eSBUAVType : uint8_t
	{
		SB_TYPE_UAV_INSTANCE = BIND_SLOT_UAV_END,
		SB_TYPE_UAV_RESERVED_1,
		SB_TYPE_UAV_RESERVED_2,
		SB_TYPE_UAV_END,
	};

	enum eSampler : uint8_t
	{
		SAMPLER_TEXTURE = 0,
		SAMPLER_SHADOW,
		SAMPLER_END,
	};

	enum eToolkitRenderType : uint8_t
	{
		TOOLKIT_RENDER_UNKNOWN = 0,
		TOOLKIT_RENDER_PRIMITIVE = 1,
		TOOLKIT_RENDER_SPRITE,
	};

	enum eRasterizerSlot : uint8_t
	{
		RASTERIZER_SLOT_SRV,
		RASTERIZER_SLOT_CB,
		RASTERIZER_SLOT_UAV,
		RASTERIZER_SLOT_SAMPLER,
		RASTERIZER_SLOT_COUNT
	};

	enum eCBType : uint8_t
	{
		CB_TYPE_WVP = 0,
		CB_TYPE_PARAM,
		CB_TYPE_END,
	};

	enum eRaytracingCBType : uint8_t
	{
		RAYTRACING_CB_VIEWPORT = 0,
		RAYTRACING_CB_COUNT
	};

	enum eRaytracingCBLocalType : uint8_t
	{
		RAYTRACING_CB_LOCAL_MATERIAL = 0,
		RAYTRACING_CB_LOCAL_COUNT
	};

	constexpr UINT g_max_cb_slots = CB_TYPE_END;
	constexpr UINT g_max_engine_texture_slots = RESERVED_TEX_END;
	constexpr UINT g_max_uav_slots = SB_TYPE_UAV_END;
	constexpr UINT g_max_sampler_slots = SAMPLER_END;
	constexpr UINT g_total_engine_slots = g_max_engine_texture_slots + g_max_cb_slots + g_max_uav_slots;

	constexpr UINT g_srv_offset = 0;
	constexpr UINT g_cb_offset = g_max_engine_texture_slots;
	constexpr UINT g_uav_offset = g_cb_offset + g_max_cb_slots;

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
		SHADER_DEPTH_TEST_ALL = 1,

		SHADER_DEPTH_NEVER = 2,
		SHADER_DEPTH_LESS = 4,
		SHADER_DEPTH_EQUAL = 8,
		SHADER_DEPTH_LESS_EQUAL = 16,
		SHADER_DEPTH_GREATER = 32,
		SHADER_DEPTH_NOT_EQUAL = 64,
		SHADER_DEPTH_GREATER_EQUAL = 128,
		SHADER_DEPTH_ALWAYS = 256,
	};

	enum eShaderSampler : UINT
	{
		SHADER_SAMPLER_CLAMP = 0,
		SHADER_SAMPLER_WRAP = 1,
		SHADER_SAMPLER_MIRROR = 2,
		SHADER_SAMPLER_BORDER = 4,
		SHADER_SAMPLER_MIRROR_ONCE = 8,
		shader_sampler_address_mask = 15,

		SHADER_SAMPLER_NEVER = 16,
		SHADER_SAMPLER_LESS = 32,
		SHADER_SAMPLER_EQUAL = 64,
		SHADER_SAMPLER_LESS_EQUAL = 128,
		SHADER_SAMPLER_GREATER = 256,
		SHADER_SAMPLER_NOT_EQUAL = 512,
		SHADER_SAMPLER_GREATER_EQUAL = 1024,
		SHADER_SAMPLER_ALWAYS = 2048,
	};

	enum eShaderRasterizer : UINT
	{
		SHADER_RASTERIZER_CULL_NONE = 0,
		SHADER_RASTERIZER_CULL_FRONT = 1,
		SHADER_RASTERIZER_CULL_BACK = 2,
		SHADER_RASTERIZER_FILL_WIREFRAME = 4,
		SHADER_RASTERIZER_FILL_SOLID = 8,
	};

	using eShaderDepths = UINT;
	using eShaderRasterizers = UINT;
	using eShaderSamplers = UINT;
}
#endif

// Static structured buffer type, this should be added to every structured buffer
#define SB_T(enum_val) static constexpr eSBType sbtype = enum_val;
#define CLIENT_SB_T(enum_val) static constexpr eClientSBType csbtype = enum_val;

// Static structured buffer UAV type, this should be added to every structured buffer UAV
#define CLIENT_SB_UAV_T(enum_val) static constexpr eClientSBUAVType csbuavtype = enum_val;
#define SB_UAV_T(enum_val) static constexpr eSBUAVType sbuavtype = enum_val;

namespace Engine
{
	template <typename T, typename U>
	void CheckSize(const U compare_value, const std::wstring_view out_string)
	{
#if WITH_DEBUG
		if (compare_value > std::numeric_limits<T>::max() || compare_value < std::numeric_limits<T>::min())
		{
			OutputDebugStringW(out_string.data());
		}
#endif
	}

	template <typename T>
	struct OffsetT
	{
		T     value;
		float ___p[(16 / sizeof(T)) - 1]{};

		OffsetT()
			: value(),
			___p{}
		{
			static_assert(sizeof(T) <= 16, "OffsetT: sizeof(T) > 16");
		}

		~OffsetT() = default;

		OffsetT(const T& v)
			: value(v) {}

		OffsetT& operator=(const T& v)
		{
			value = v;
			return *this;
		}

		friend class boost::serialization::access;

		template <class Archive>
		void serialize(Archive& ar, const unsigned int file_version)
		{
			ar& value;
			ar& ___p;
		}
	};

	enum eShaderType : uint8_t
	{
		SHADER_VERTEX = 0,
		SHADER_PIXEL,
		SHADER_GEOMETRY,
		SHADER_COMPUTE,
		SHADER_HULL,
		SHADER_DOMAIN,
		SHADER_UNKNOWN
	};

	enum eClientSBType : uint8_t;
	enum eClientSBUAVType : uint8_t;

	template <typename T>
	using Weak = boost::weak_ptr<T>;

	template <typename T>
	using Strong = boost::shared_ptr<T>;

	using GenericString = std::string;
	using EntityName = GenericString;
	using TypeName = GenericString;
	using MetadataPathStr = GenericString;
	using MetadataPath = std::filesystem::path;

	using IDType = uint32_t;
	using GlobalEntityID = IDType;
	using LocalActorID = IDType;
	using LocalComponentID = IDType;
	using LocalResourceID = IDType;

	using LayerSizeType = uint32_t;
	using ScriptSizeType = uint32_t;

	inline constexpr static IDType g_invalid_id = -1;

    using UINT = uint32_t;

	enum eResourceType : uint8_t;
	enum eComponentType : uint8_t;
	enum eDefObjectType : uint8_t;
	enum eTaskType : uint8_t;

	class Serializer;
	struct ComponentPriorityComparer;

	using CommandDescriptorLambda = std::function<void(const Weak<CommandPair>&, const DescriptorPtr&)>;

	template <typename WeakT, typename BoundingValueGetter, float Epsilon>
	class Octree;

	namespace Objects
	{
		class Light;
		class Camera;
		class Text;
		class Observer;
	} // namespace Objects

	namespace Components
	{
		namespace Abstracts
		{
			class RenderComponent;
		}

		class Collider;
		class OffsetCollider;
		class Transform;
		class Rigidbody;
		class ObserverController;
		class SoundPlayer;
		class ModelRenderer;
		class Animator;
		class ParticleRenderer;
	} // namespace Component

	class Script;
	class Scene;
	class Layer;

	namespace Graphics
	{
		template <typename T>
		class StructuredBuffer;

		struct AnimationPrimitive;
		struct BonePrimitive;
		struct BoneAnimationPrimitive;
		struct VertexElement;

		namespace SBs
		{
			struct InstanceSB;
			struct InstanceParticleSB;
			struct LocalParamSB;
			struct InstanceModelSB;
			struct MaterialSB;
		} // namespace SBs
	} // namespace Graphic

	using VertexCollection = std::vector<Graphics::VertexElement>;
	using IndexCollection = std::vector<uint32_t>;

	namespace Resources
	{
		class Prefab;
		class Font;
		class Mesh;
		class Sound;
		class Texture;
		class BoneAnimation;
		class Shape;
		class Bone;
		class BaseAnimation;
		class Material;
		class Shader;
		class AnimationTexture;
		class ShadowTexture;
		class Texture1D;
		class Texture2D;
		class Texture3D;
		class ComputeShader;
	} // namespace Resources

	namespace Abstracts
	{
		class Entity;
		class ObjectBase;
		class Component;
		class Actor;
		class Renderable;
		class Resource;

		template <typename T, typename... Args>
		class Singleton;
	} // namespace Abstracts

	namespace Managers
	{
		class RaytracingPipeline;
		class Raytracer;
		class ToolkitAPI;
		class RenderPipeline;
		class D3Device;
		class ShadowManager;
		class ReflectionEvaluator;
		class Renderer;
		class ImGuiManager;

		class PhysicsManager;
		class LerpManager;
		class ConstraintSolver;
		class CollisionDetector;
		class Graviton;

		class ProjectionFrustum;
		class Application;
		class ResourceManager;
		class SceneManager;
		class Debugger;
		class MouseManager;
		class TaskScheduler;
	} // namespace Managers

	using ObjectPredication = std::function<bool(const Strong<Abstracts::ObjectBase>&)>;
}
