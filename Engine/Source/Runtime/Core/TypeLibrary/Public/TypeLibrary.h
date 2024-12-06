#pragma once
#include <boost/smart_ptr.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/serialization/access.hpp>
#include <string>
#include <filesystem>

#define DLLIMPORT __declspec(dllimport)
#define DLLEXPORT __declspec(dllexport)

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

constexpr DXGI_FORMAT g_default_rtv_format = DXGI_FORMAT_R8G8B8A8_UNORM;

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

namespace Engine::Graphics
{
	struct ParamBase
	{
	public:
		constexpr ParamBase() = default;

		template <typename T>
		void SetParam(const size_t slot, const T& param)
		{
			if constexpr (std::is_same_v<T, int>)
			{
				i_param[slot] = param;
			}
			else if constexpr (std::is_same_v<T, UINT>)
			{
				i_param[slot] = static_cast<int>(param);
			}
			else if constexpr (std::is_same_v<T, float>)
			{
				f_param[slot] = param;
			}
			else if constexpr (std::is_same_v<T, Vector3>)
			{
				std::memcpy(&v_param[slot], &param, sizeof(Vector3));
			}
			else if constexpr (std::is_same_v<T, Vector4>)
			{
				_mm_store_ps(v_param[slot].x.m128_f32, _mm_load_ps(param.x));
			}
			else if constexpr (std::is_same_v<T, Matrix>)
			{
				const auto row0 = const_cast<float*>(&param.m[0][0]);
				const auto row2 = const_cast<float*>(&param.m[2][0]);

				_mm256_store_ps(m_param[slot].m[0], _mm256_load_ps(row0));
				_mm256_store_ps(m_param[slot].m[2], _mm256_load_ps(row2));
			}
			else
			{
				throw std::runtime_error("Invalid type");
			}
		}

		template <typename T>
		T& GetParam(const size_t slot)
		{
			if constexpr (std::is_same_v<T, int>)
			{
				return i_param[slot];
			}
			else if constexpr (std::is_same_v<T, UINT>)
			{
				return reinterpret_cast<UINT&>(i_param[slot]);
			}
			else if constexpr (std::is_same_v<T, bool>)
			{
				return reinterpret_cast<bool&>(i_param[slot]);
			}
			else if constexpr (std::is_same_v<T, float>)
			{
				return f_param[slot];
			}
			else if constexpr (std::is_same_v<T, Vector3>)
			{
				return reinterpret_cast<Vector3&>(v_param[slot]);
			}
			else if constexpr (std::is_same_v<T, Vector4>)
			{
				return reinterpret_cast<Vector4&>(v_param[slot]);
			}
			else if constexpr (std::is_same_v<T, Matrix>)
			{
				return reinterpret_cast<Matrix&>(m_param[slot]);
			}
			else
			{
				throw std::runtime_error("Invalid type");
			}
		}

		template <typename T>
		T GetParam(const size_t slot) const
		{
			if constexpr (std::is_same_v<T, int>)
			{
				return i_param[slot];
			}
			else if constexpr (std::is_same_v<T, bool>)
			{
				return static_cast<bool>(i_param[slot]);
			}
			else if constexpr (std::is_same_v<T, UINT>)
			{
				return static_cast<UINT>(i_param[slot]);
			}
			else if constexpr (std::is_same_v<T, float>)
			{
				return f_param[slot];
			}
			else if constexpr (std::is_same_v<T, Vector3>)
			{
				return v_param[slot];
			}
			else if constexpr (std::is_same_v<T, Vector4>)
			{
				return v_param[slot];
			}
			else if constexpr (std::is_same_v<T, Matrix>)
			{
				Matrix m;
				_mm256_store_ps(m.m[0], m_param[slot].m[0]);
				_mm256_store_ps(m.m[2], m_param[slot].m[1]);
				return m;
			}
			else
			{
				throw std::runtime_error("Invalid type");
			}
		}

	private:
		friend class boost::serialization::access;

		template <class Archive>
		void serialize(Archive& ar, const unsigned int file_version)
		{
			ar& f_param;
			ar& i_param;
			ar& v_param;
			ar& m_param;
		}

		constexpr static size_t max_param = 8;

		float            f_param[max_param * (sizeof(Vector4) / sizeof(float))]{};
		int              i_param[max_param * (sizeof(Vector4) / sizeof(float))]{};
		Vector4 v_param[max_param]{};
		Matrix  m_param[max_param]{};
	};

	static_assert(sizeof(ParamBase) % sizeof(Vector4) == 0);
	static_assert(sizeof(ParamBase) < 2048);
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
	using MetadataPath = boost::filesystem::path;

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

		template <typename T>
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
		class SoundManager;
		class InputManager;

		class PhysicsManager;
		class LerpManager;
		class ConstraintSolver;
		class CollisionDetector;
		class Graviton;

		class ProjectionFrustum;
		class EngineEntryPoint;
		class ResourceManager;
		class SceneManager;
		class Debugger;
		class InputManager;
		class TaskScheduler;
	} // namespace Managers

	using ObjectPredication = std::function<bool(const Strong<Abstracts::ObjectBase>&)>;
}
