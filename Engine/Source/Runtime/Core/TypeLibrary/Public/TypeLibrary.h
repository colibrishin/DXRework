#pragma once
#include <memory>
#include <string>
#include <filesystem>

#include "CoreEntity.h"
#include "Allocator/Public/Allocator.h"
#include "CoreType.h"

#if defined(USE_DX12)
#include <directxtk12/SimpleMath.h>
#include <directxtk12/SimpleMath.inl>
#include <wrl/client.h>

namespace Engine
{
	using DirectX::SimpleMath::Vector2;
	using DirectX::SimpleMath::Vector3;
	using DirectX::SimpleMath::Vector4;
	using DirectX::SimpleMath::Color;
	using DirectX::SimpleMath::Quaternion;
	using DirectX::SimpleMath::Ray;
	using DirectX::SimpleMath::Matrix;
	using DirectX::BoundingBox;
	using DirectX::BoundingFrustum;
	using DirectX::BoundingOrientedBox;
	using DirectX::BoundingSphere;
	using DirectX::XMFLOAT2;
	using DirectX::XMFLOAT3X3;
	using DirectX::XMVECTORF32;
	using DirectX::BoundingBox;
	using DirectX::BoundingFrustum;
	using DirectX::BoundingOrientedBox;
	using DirectX::BoundingSphere;
	using DirectX::XMFLOAT2;
	using DirectX::XMFLOAT3X3;
	using DirectX::XMVECTORF32;
	using Microsoft::WRL::ComPtr;
}

using Engine::Vector2;
using Engine::Vector3;
using Engine::Vector4;
using Engine::Color;
using Engine::Quaternion;
using Engine::Ray;
using Engine::Matrix;
using Engine::BoundingBox;
using Engine::BoundingFrustum;
using Engine::BoundingOrientedBox;
using Engine::BoundingSphere;
using Engine::BoundingBox;
using Engine::BoundingFrustum;
using Engine::BoundingOrientedBox;
using Engine::BoundingSphere;

inline constexpr static Engine::Vector3 g_forward = { 0.f, 0.f, 1.f };
inline constexpr static Engine::Vector3 g_backward = { 0.f, 0.f, -1.f };

namespace boost::serialization
{
	template <typename Archive>
	void serialize(Archive& ar, DirectX::SimpleMath::Quaternion& x, const unsigned int /*version*/)
	{
		ar& x.x;
		ar& x.y;
		ar& x.z;
		ar& x.w;
	}

	template <typename Archive>
	void serialize(Archive& ar, DirectX::SimpleMath::Matrix& x, const unsigned int /*version*/)
	{
		ar & x._11;
		ar & x._12;
		ar & x._13;
		ar & x._14;
		ar & x._21;
		ar & x._22;
		ar & x._23;
		ar & x._24;
		ar & x._31;
		ar & x._32;
		ar & x._33;
		ar & x._34;
		ar & x._41;
		ar & x._42;
		ar & x._43;
		ar & x._44;
	}

	template <typename Archive>
	void serialize(Archive& ar, DirectX::SimpleMath::Vector4& x, const unsigned int /*version*/)
	{
		ar & x.x;
		ar & x.y;
		ar & x.z;
		ar & x.w;
	}
	
	template <typename Archive>
	void serialize(Archive& ar, DirectX::SimpleMath::Color& x, const unsigned int /*version*/)
	{
		ar & x.x;
		ar & x.y;
		ar & x.z;
		ar & x.w;
	}

	template <typename Archive>
	void serialize(Archive& ar, DirectX::SimpleMath::Vector3& x, const unsigned int /*version*/)
	{
		ar & x.x;
		ar & x.y;
		ar & x.z;
	}

	template <typename Archive>
	void serialize(Archive& ar, DirectX::SimpleMath::Vector2& x, const unsigned int /*version*/)
	{
		ar & x.x;
		ar & x.y;
	}

	template <typename Archive>
	void serialize(Archive& ar, DirectX::XMFLOAT3X3& x, const unsigned int /*version*/)
	{
		ar & x._11;
		ar & x._12;
		ar & x._13;
		ar & x._21;
		ar & x._22;
		ar & x._23;
		ar & x._31;
		ar & x._32;
		ar & x._33;
	}

	template <typename Archive>
	void serialize(Archive& ar, DirectX::XMFLOAT2& x, const unsigned int /*version*/)
	{
		ar & x.x;
		ar & x.y;
	}

	template <typename Archive>
	void serialize(Archive& ar, DirectX::XMFLOAT3& x, const unsigned int /*version*/)
	{
		ar & x.x;
		ar & x.y;
		ar & x.z;
	}

	template <typename Archive>
	void serialize(Archive& ar, DirectX::XMFLOAT4& x, const unsigned int /*version*/)
	{
		ar & x.x;
		ar & x.y;
		ar & x.z;
		ar & x.w;
	}

	template <typename Archive>
	void serialize(Archive& ar, DirectX::BoundingBox& bb, const unsigned int /*version*/)
	{
		ar & bb.Center;
		ar & bb.Extents;
	}

	template <typename Archive>
	void serialize(Archive& ar, DirectX::BoundingOrientedBox& obb, const unsigned int /*version*/)
	{
		ar & obb.Center;
		ar & obb.Orientation;
		ar & obb.Extents;
	}

	template <typename Archive>
	void serialize(Archive& ar, DirectX::BoundingSphere& bs, const unsigned int /*version*/)
	{
		ar & bs.Center;
		ar & bs.Radius;
	}
}
#endif

namespace Engine::Graphics
{
	struct ENGINE_CORE_API ParamBase
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
				_mm_store_ps(v_param[slot].x, _mm_load_ps(param.x));
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
				return m_param[slot];
			}
			else
			{
				throw std::runtime_error("Invalid type");
			}
		}

		template <typename T> requires (std::is_same_v<float, T> || std::is_same_v<int, T> || std::is_same_v<Vector4, T> || std::is_same_v<Matrix, T>)
		T* EvaluateAddress(const size_t slot)
		{
			auto cast_address = reinterpret_cast<char*>(this);
			
			if constexpr (std::is_same_v<float, T>)
			{
				assert(slot < max_param * (sizeof(Vector4) / sizeof(float)));
				return reinterpret_cast<float*>(cast_address + float_section_begin) + slot;
			}
			else if constexpr (std::is_same_v<int, T>)
			{
				assert(slot < max_param * (sizeof(Vector4) / sizeof(float)));
				return reinterpret_cast<int*>(cast_address + int_section_begin) + slot;
			}
			else if constexpr (std::is_same_v<Vector4, T>)
			{
				assert(slot < max_param);
				return reinterpret_cast<Vector4*>(cast_address + vector_section_begin) + slot;
			}
			else if constexpr (std::is_same_v<Matrix, T>)
			{
				assert(slot < max_param);
				return reinterpret_cast<Matrix*>(cast_address + matrix_section_begin) + slot;
			}
			else
			{
				assert(false, "Invalid type");
			}
			return nullptr; // Suppressing the warning
		}

	private:
		friend class boost::serialization::access;
		template <typename Archive>
		void serialize(Archive& ar, const unsigned int /*version*/)
		{
			ar& f_param;
			ar& i_param;
			ar& v_param;
			ar& m_param;
		}
		
		constexpr static size_t max_param = 8;

		float   f_param[max_param * (sizeof(Vector4) / sizeof(float))]{};
		int     i_param[max_param * (sizeof(Vector4) / sizeof(float))]{};
		Vector4 v_param[max_param]{};
		Matrix  m_param[max_param]{};

		constexpr static size_t float_section_begin = 0;
		constexpr static size_t int_section_begin = float_section_begin + sizeof(i_param);
		constexpr static size_t vector_section_begin = int_section_begin + sizeof(v_param);
		constexpr static size_t matrix_section_begin = vector_section_begin + sizeof(m_param);
	};

	static_assert(sizeof(ParamBase) % sizeof(Vector4) == 0);
	static_assert(sizeof(ParamBase) < 2048);
}

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

	typedef ENGINE_CORE_API UINT eComponentUpdatePriorities;

	enum ENGINE_CORE_API eComponentUpdatePriority : UINT
	{
		COM_PRIORITY_POSITIONAL = 0,
		COM_PRIORITY_PHYSICS = 1 << 1,
		COM_PRIORITY_RENDER = 1 << 2
	};

	enum ENGINE_CORE_API eBindType : uint8_t
	{
		BIND_TYPE_SAMPLER = 0,
		BIND_TYPE_CB,
		BIND_TYPE_UAV,
		BIND_TYPE_SRV,
		BIND_TYPE_RTV,
		BIND_TYPE_DSV,
		BIND_TYPE_COUNT
	};

	enum ENGINE_CORE_API eTexBindSlot : uint8_t
	{
		BIND_SLOT_TEX = 0,
		BIND_SLOT_TEXARR = BIND_SLOT_TEX + 16,
		BIND_SLOT_TEXCUBE = BIND_SLOT_TEXARR + 2,
		BIND_SLOT_TEX1D = BIND_SLOT_TEXCUBE + 2,
		BIND_SLOT_END = BIND_SLOT_TEX1D + 2,
	};

	static constexpr size_t g_max_texture_per_material = 8;

	enum ENGINE_CORE_API eSBType : uint8_t
	{
		SB_TYPE_LIGHT = BIND_SLOT_END,
		SB_TYPE_LIGHT_VP,
		SB_TYPE_INSTANCE,
		SB_TYPE_LOCAL_PARAM,
		SB_TYPE_END
	};

	enum ENGINE_CORE_API eRaytracingGlobalSBType
	{
	    SB_TYPE_RAYTRACING_GLOBAL_INSTANCE,
	    SB_TYPE_RAYTRACING_GLOBAL_LIGHT,
		SB_TYPE_RAYTRACING_GLOBAL_END
	};

    enum ENGINE_CORE_API eRaytracingLocalSBType
    {
        SB_TYPE_RAYTRACING_LOCAL_VERTEX,
        SB_TYPE_RAYTRACING_LOCAL_INDEX,
        SB_TYPE_RAYTRACING_LOCAL_END
    };

	enum ENGINE_CORE_API eReservedTexBindSlot : uint8_t
	{
		RESERVED_TEX_RENDERED = SB_TYPE_END,
		RESERVED_TEX_SHADOW_MAP,
		RESERVED_TEX_END = RESERVED_TEX_SHADOW_MAP + CFG_MAX_DIRECTIONAL_LIGHT,
	};

	enum ENGINE_CORE_API eReservedTexUserTexBindSlot : uint8_t
	{
		RESERVED_USER_TEX_BEGIN = RESERVED_TEX_END,
		RESERVED_USER_TEX_BONES = RESERVED_USER_TEX_BEGIN,
		RESERVED_USER_TEX_ATLAS,
		RESERVED_USER_TEX_END,
	};

	enum ENGINE_CORE_API eTexUAVBindSlot : uint8_t
	{
		BIND_SLOT_UAV_TEX_1D = 0,
		BIND_SLOT_UAV_TEX_2D = BIND_SLOT_UAV_TEX_1D + 2,
		BIND_SLOT_UAV_TEXARR = BIND_SLOT_UAV_TEX_2D + 2,
		BIND_SLOT_UAV_END = BIND_SLOT_UAV_TEXARR + 2,
	};

	enum ENGINE_CORE_API eSBUAVType : uint8_t
	{
		SB_TYPE_UAV_INSTANCE = BIND_SLOT_UAV_END,
		SB_TYPE_UAV_RESERVED_1,
		SB_TYPE_UAV_RESERVED_2,
		SB_TYPE_UAV_END,
	};

	enum ENGINE_CORE_API eSampler : uint8_t
	{
		SAMPLER_TEXTURE = 0,
		SAMPLER_SHADOW,
		SAMPLER_END,
	};

	enum ENGINE_CORE_API eRasterizerSlot : uint8_t
	{
		RASTERIZER_SLOT_SRV,
		RASTERIZER_SLOT_CB,
		RASTERIZER_SLOT_UAV,
		RASTERIZER_SLOT_SAMPLER,
		RASTERIZER_SLOT_COUNT
	};

    enum ENGINE_CORE_API eRaytracingGlobalSlot : uint8_t
    {
        RAYTRACING_GLOBAL_SLOT_TLAS,
        RAYTRACING_GLOBAL_SLOT_LIGHT,
        RAYTRACING_GLOBAL_SLOT_INSTANCE,
        RAYTRACING_GLOBAL_SLOT_OUTPUT,
        RAYTRACING_GLOBAL_SLOT_WVP,
        RAYTRACING_GLOBAL_SLOT_PARAM,
        RAYTRACING_GLOBAL_SLOT_COUNT
    };
    
    enum ENGINE_CORE_API eRaytracingLocalSlot : uint8_t
    {
        RAYTRACING_LOCAL_SLOT_SRV,
        RAYTRACING_LOCAL_SLOT_UAV,
        RAYTRACING_LOCAL_SLOT_SAMPLER,
        RAYTRACING_LOCAL_SLOT_RANGE_END,
        RAYTRACING_LOCAL_SLOT_VERTEX = RAYTRACING_LOCAL_SLOT_RANGE_END,
        RAYTRACING_LOCAL_SLOT_INDEX,
        RAYTRACING_LOCAL_SLOT_END
    };

	enum ENGINE_CORE_API eCBType : uint8_t
	{
		CB_TYPE_WVP = 0,
		CB_TYPE_PARAM,
		CB_TYPE_END,
	};

	constexpr UINT g_max_cb_slots = CB_TYPE_END;
	constexpr UINT g_max_engine_texture_slots = RESERVED_USER_TEX_END;
	constexpr UINT g_max_uav_slots = SB_TYPE_UAV_END;
	constexpr UINT g_max_sampler_slots = SAMPLER_END;
	constexpr UINT g_total_engine_slots = g_max_engine_texture_slots + g_max_cb_slots + g_max_uav_slots;

	constexpr UINT g_srv_offset = 0;
	constexpr UINT g_cb_offset = g_max_engine_texture_slots;
	constexpr UINT g_uav_offset = g_cb_offset + g_max_cb_slots;

    constexpr UINT g_local_raytracing_srv_offset = 0;
    constexpr UINT g_local_raytracing_uav_offset = g_max_engine_texture_slots;
    constexpr UINT g_local_raytracing_total_engine_slots = g_max_engine_texture_slots + g_max_uav_slots;

	enum ENGINE_CORE_API eToolkitRenderType : uint8_t
	{
		TOOLKIT_RENDER_UNKNOWN = 0,
		TOOLKIT_RENDER_PRIMITIVE = 1,
		TOOLKIT_RENDER_SPRITE,
	};

	enum ENGINE_CORE_API eShaderDomain : UINT
	{
		SHADER_DOMAIN_BEGIN = 0,
		SHADER_DOMAIN_OPAQUE = SHADER_DOMAIN_BEGIN,
		SHADER_DOMAIN_MASK,
		SHADER_DOMAIN_TRANSPARENT,
		SHADER_DOMAIN_POST_PROCESS,
		SHADER_DOMAIN_MAX,
	};

	enum ENGINE_CORE_API eShaderDepthMode : uint8_t
	{
		SHADER_DEPTH_TEST_ZERO = 0,
		SHADER_DEPTH_TEST_ALL = 1,
	};

	enum ENGINE_CORE_API eShaderDepthFunction : UINT
	{
		SHADER_DEPTH_NONE = 0,
		SHADER_DEPTH_NEVER = 1,
		SHADER_DEPTH_LESS = 2,
		SHADER_DEPTH_EQUAL = 3,
		SHADER_DEPTH_LESS_EQUAL = 4,
		SHADER_DEPTH_GREATER = 5,
		SHADER_DEPTH_NOT_EQUAL = 6,
		SHADER_DEPTH_GREATER_EQUAL = 7,
		SHADER_DEPTH_ALWAYS = 8
	};

	enum ENGINE_CORE_API eShaderSamplerAddress : UINT
	{
		SHADER_SAMPLER_CLAMP = 1,
		SHADER_SAMPLER_WRAP = 2,
		SHADER_SAMPLER_MIRROR = 3,
		SHADER_SAMPLER_BORDER = 4,
		SHADER_SAMPLER_MIRROR_ONCE = 5
	};

	enum ENGINE_CORE_API eShaderSamplerFunction : UINT
	{
		D3D12_COMPARISON_FUNC_NONE	= 0,
		SHADER_SAMPLER_NEVER = 1,
		SHADER_SAMPLER_LESS = 2,
		SHADER_SAMPLER_EQUAL = 3,
		SHADER_SAMPLER_LESS_EQUAL = 4,
		SHADER_SAMPLER_GREATER = 5,
		SHADER_SAMPLER_NOT_EQUAL = 6,
		SHADER_SAMPLER_GREATER_EQUAL = 7,
		SHADER_SAMPLER_ALWAYS = 8,
	};

	enum ENGINE_CORE_API eShaderRasterizerCull : uint8_t
	{
		SHADER_RASTERIZER_CULL_NONE = 1,
		SHADER_RASTERIZER_CULL_FRONT = 2,
		SHADER_RASTERIZER_CULL_BACK = 3,
	};

	enum ENGINE_CORE_API eShaderRasterizerDraw : uint8_t
	{
		SHADER_RASTERIZER_FILL_WIREFRAME = 2,
		SHADER_RASTERIZER_FILL_SOLID = 3,
	};

	using eShaderDepths = UINT;
	using eShaderRasterizers = UINT;
	using eShaderSamplers = UINT;

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

	private:
		friend class boost::serialization::access;
		
		template <typename Archive>
		void serialize(Archive& ar, const unsigned int /*version*/)
		{
			ar& value;
		}
	};

	enum ENGINE_CORE_API eShaderType : uint8_t
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

	using LocalActorID = IDType;
	using LocalComponentID = IDType;
	using LocalResourceID = IDType;

	using LayerSizeType = uint32_t;
	using ScriptSizeType = uint32_t;

	inline constexpr static IDType g_invalid_id = -1;

    using UINT = uint32_t;

	using ResourceType = HashType;
	using ComponentType = HashType;
	using ScriptType = HashType;
	enum eTaskType : uint8_t;

	class Serializer;
	struct ComponentPriorityComparer;
	struct bounding_getter;

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
			class ShapeRenderComponent;
		}

		class Collider;
		class Transform;
		class Rigidbody;
		class ObserverController;
		class TextRenderer;
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
	    class ShaderBase;
	    class RaytracingShader;
		class AnimationTexture;
		class ShadowTexture;
		class Texture1D;
		class Texture2D;
		class Texture3D;
		class ComputeShader;
	} // namespace Resources

	namespace Abstracts
	{
		class ObjectBase;
		class Component;
		class Actor;
		class Resource;
	} // namespace Abstracts

	namespace Managers
	{
		class RaytracingPipeline;
		class Raytracer;
		class ToolkitAPI;
		class RenderPipeline;
		class ShadowManager;
		class ReflectionEvaluator;
		class Renderer;
		class ImGuiManager;
		class SoundManager;
		class InputManager;
		class CameraManager;

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

	template <typename WeakT, typename BoundingValueGetter, float Epsilon>
	class octree_impl;

	using Octree = octree_impl<Weak<Abstracts::ObjectBase>, bounding_getter, CFG_EPSILON>;

	using ObjectPredication = std::function<bool(const Strong<Abstracts::ObjectBase>&)>;

	using WeakObjGlobalMap = fast_pool_unordered_map<GlobalEntityID, Weak<Abstracts::ObjectBase>>;
	using WeakObjVec = aligned_vector<Weak<Abstracts::ObjectBase>>;
	using LocalGlobalIDMap = fast_pool_unordered_map<LocalActorID, GlobalEntityID>;
	using WeakComVec = aligned_vector<Weak<Abstracts::Component>>;
	using WeakComMap = fast_pool_unordered_map<GlobalEntityID, Weak<Abstracts::Component>>;
	using WeakScpVec = aligned_vector<Weak<Script>>;
	using WeakScpMap = fast_pool_unordered_map<GlobalEntityID, Weak<Script>>;
	using WeakComRootMap = fast_pool_unordered_map<ComponentType, WeakComMap>;
	using WeakScpRootMap = fast_pool_unordered_map<ScriptType, WeakScpMap>;

}
