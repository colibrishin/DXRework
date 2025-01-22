#pragma once
#include <boost/smart_ptr.hpp>
#include <boost/serialization/access.hpp>
#include <string>
#include <filesystem>
#include "Source/Runtime/Misc.h"

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

	enum CORE_API eBindType : uint8_t
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

	enum CORE_API eTexBindSlot : uint8_t
	{
		BIND_SLOT_TEX = 0,
		BIND_SLOT_TEXARR = BIND_SLOT_TEX + 4,
		BIND_SLOT_TEXCUBE = BIND_SLOT_TEXARR + 2,
		BIND_SLOT_TEX1D = BIND_SLOT_TEXCUBE + 2,
		BIND_SLOT_END = BIND_SLOT_TEX1D + 2,
	};

	enum CORE_API eSBType : uint8_t
	{
		SB_TYPE_LIGHT = BIND_SLOT_END,
		SB_TYPE_LIGHT_VP,
		SB_TYPE_INSTANCE,
		SB_TYPE_LOCAL_PARAM,
		SB_TYPE_MATERIAL,
		SB_TYPE_END
	};

	enum CORE_API eRaytracingSBType
	{
		SB_TYPE_RAYTRACING_TLAS,
		SB_TYPE_RAYTRACING_VERTEX,
		SB_TYPE_RAYTRACING_INDEX,
		SB_TYPE_RAYTRACING_END
	};

	enum CORE_API eReservedTexBindSlot : uint8_t
	{
		RESERVED_TEX_RENDERED = SB_TYPE_END,
		RESERVED_TEX_BONES,
		RESERVED_TEX_ATLAS,
		RESERVED_TEX_SHADOW_MAP,
		RESERVED_TEX_END = RESERVED_TEX_SHADOW_MAP + CFG_MAX_DIRECTIONAL_LIGHT,
	};

	enum CORE_API eTexUAVBindSlot : uint8_t
	{
		BIND_SLOT_UAV_TEX_1D = 0,
		BIND_SLOT_UAV_TEX_2D = BIND_SLOT_UAV_TEX_1D + 2,
		BIND_SLOT_UAV_TEXARR = BIND_SLOT_UAV_TEX_2D + 2,
		BIND_SLOT_UAV_END,
	};

	enum CORE_API eSBUAVType : uint8_t
	{
		SB_TYPE_UAV_INSTANCE = BIND_SLOT_UAV_END,
		SB_TYPE_UAV_RESERVED_1,
		SB_TYPE_UAV_RESERVED_2,
		SB_TYPE_UAV_END,
	};

	enum CORE_API eSampler : uint8_t
	{
		SAMPLER_TEXTURE = 0,
		SAMPLER_SHADOW,
		SAMPLER_END,
	};

	enum CORE_API eRasterizerSlot : uint8_t
	{
		RASTERIZER_SLOT_SRV,
		RASTERIZER_SLOT_CB,
		RASTERIZER_SLOT_UAV,
		RASTERIZER_SLOT_SAMPLER,
		RASTERIZER_SLOT_COUNT
	};

	enum CORE_API eCBType : uint8_t
	{
		CB_TYPE_WVP = 0,
		CB_TYPE_PARAM,
		CB_TYPE_END,
	};

	enum CORE_API eRaytracingCBType : uint8_t
	{
		RAYTRACING_CB_VIEWPORT = 0,
		RAYTRACING_CB_COUNT
	};

	enum CORE_API eRaytracingCBLocalType : uint8_t
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

	enum CORE_API eToolkitRenderType : uint8_t
	{
		TOOLKIT_RENDER_UNKNOWN = 0,
		TOOLKIT_RENDER_PRIMITIVE = 1,
		TOOLKIT_RENDER_SPRITE,
	};

	enum CORE_API eShaderDomain : UINT
	{
		SHADER_DOMAIN_OPAQUE = 0,
		SHADER_DOMAIN_MASK,
		SHADER_DOMAIN_TRANSPARENT,
		SHADER_DOMAIN_POST_PROCESS,
		SHADER_DOMAIN_MAX,
	};

	enum CORE_API eShaderDepth : UINT
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

	enum CORE_API eShaderSampler : UINT
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

	enum CORE_API eShaderRasterizer : UINT
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
	struct CORE_API ParamBase
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

	private:
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
	};

	enum CORE_API eShaderType : uint8_t
	{
		SHADER_VERTEX = 0,
		SHADER_PIXEL,
		SHADER_GEOMETRY,
		SHADER_COMPUTE,
		SHADER_HULL,
		SHADER_DOMAIN,
		SHADER_UNKNOWN
	};

	enum CORE_API eFormat
	{
	    TEX_FORMAT_UNKNOWN	                                = 0,
	    TEX_FORMAT_R32G32B32A32_TYPELESS                   = 1,
	    TEX_FORMAT_R32G32B32A32_FLOAT                      = 2,
	    TEX_FORMAT_R32G32B32A32_UINT                       = 3,
	    TEX_FORMAT_R32G32B32A32_SINT                       = 4,
	    TEX_FORMAT_R32G32B32_TYPELESS                      = 5,
	    TEX_FORMAT_R32G32B32_FLOAT                         = 6,
	    TEX_FORMAT_R32G32B32_UINT                          = 7,
	    TEX_FORMAT_R32G32B32_SINT                          = 8,
	    TEX_FORMAT_R16G16B16A16_TYPELESS                   = 9,
	    TEX_FORMAT_R16G16B16A16_FLOAT                      = 10,
	    TEX_FORMAT_R16G16B16A16_UNORM                      = 11,
	    TEX_FORMAT_R16G16B16A16_UINT                       = 12,
	    TEX_FORMAT_R16G16B16A16_SNORM                      = 13,
	    TEX_FORMAT_R16G16B16A16_SINT                       = 14,
	    TEX_FORMAT_R32G32_TYPELESS                         = 15,
	    TEX_FORMAT_R32G32_FLOAT                            = 16,
	    TEX_FORMAT_R32G32_UINT                             = 17,
	    TEX_FORMAT_R32G32_SINT                             = 18,
	    TEX_FORMAT_R32G8X24_TYPELESS                       = 19,
	    TEX_FORMAT_D32_FLOAT_S8X24_UINT                    = 20,
	    TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS                = 21,
	    TEX_FORMAT_X32_TYPELESS_G8X24_UINT                 = 22,
	    TEX_FORMAT_R10G10B10A2_TYPELESS                    = 23,
	    TEX_FORMAT_R10G10B10A2_UNORM                       = 24,
	    TEX_FORMAT_R10G10B10A2_UINT                        = 25,
	    TEX_FORMAT_R11G11B10_FLOAT                         = 26,
	    TEX_FORMAT_R8G8B8A8_TYPELESS                       = 27,
	    TEX_FORMAT_R8G8B8A8_UNORM                          = 28,
	    TEX_FORMAT_R8G8B8A8_UNORM_SRGB                     = 29,
	    TEX_FORMAT_R8G8B8A8_UINT                           = 30,
	    TEX_FORMAT_R8G8B8A8_SNORM                          = 31,
	    TEX_FORMAT_R8G8B8A8_SINT                           = 32,
	    TEX_FORMAT_R16G16_TYPELESS                         = 33,
	    TEX_FORMAT_R16G16_FLOAT                            = 34,
	    TEX_FORMAT_R16G16_UNORM                            = 35,
	    TEX_FORMAT_R16G16_UINT                             = 36,
	    TEX_FORMAT_R16G16_SNORM                            = 37,
	    TEX_FORMAT_R16G16_SINT                             = 38,
	    TEX_FORMAT_R32_TYPELESS                            = 39,
	    TEX_FORMAT_D32_FLOAT                               = 40,
	    TEX_FORMAT_R32_FLOAT                               = 41,
	    TEX_FORMAT_R32_UINT                                = 42,
	    TEX_FORMAT_R32_SINT                                = 43,
	    TEX_FORMAT_R24G8_TYPELESS                          = 44,
	    TEX_FORMAT_D24_UNORM_S8_UINT                       = 45,
	    TEX_FORMAT_R24_UNORM_X8_TYPELESS                   = 46,
	    TEX_FORMAT_X24_TYPELESS_G8_UINT                    = 47,
	    TEX_FORMAT_R8G8_TYPELESS                           = 48,
	    TEX_FORMAT_R8G8_UNORM                              = 49,
	    TEX_FORMAT_R8G8_UINT                               = 50,
	    TEX_FORMAT_R8G8_SNORM                              = 51,
	    TEX_FORMAT_R8G8_SINT                               = 52,
	    TEX_FORMAT_R16_TYPELESS                            = 53,
	    TEX_FORMAT_R16_FLOAT                               = 54,
	    TEX_FORMAT_D16_UNORM                               = 55,
	    TEX_FORMAT_R16_UNORM                               = 56,
	    TEX_FORMAT_R16_UINT                                = 57,
	    TEX_FORMAT_R16_SNORM                               = 58,
	    TEX_FORMAT_R16_SINT                                = 59,
	    TEX_FORMAT_R8_TYPELESS                             = 60,
	    TEX_FORMAT_R8_UNORM                                = 61,
	    TEX_FORMAT_R8_UINT                                 = 62,
	    TEX_FORMAT_R8_SNORM                                = 63,
	    TEX_FORMAT_R8_SINT                                 = 64,
	    TEX_FORMAT_A8_UNORM                                = 65,
	    TEX_FORMAT_R1_UNORM                                = 66,
	    TEX_FORMAT_R9G9B9E5_SHAREDEXP                      = 67,
	    TEX_FORMAT_R8G8_B8G8_UNORM                         = 68,
	    TEX_FORMAT_G8R8_G8B8_UNORM                         = 69,
	    TEX_FORMAT_BC1_TYPELESS                            = 70,
	    TEX_FORMAT_BC1_UNORM                               = 71,
	    TEX_FORMAT_BC1_UNORM_SRGB                          = 72,
	    TEX_FORMAT_BC2_TYPELESS                            = 73,
	    TEX_FORMAT_BC2_UNORM                               = 74,
	    TEX_FORMAT_BC2_UNORM_SRGB                          = 75,
	    TEX_FORMAT_BC3_TYPELESS                            = 76,
	    TEX_FORMAT_BC3_UNORM                               = 77,
	    TEX_FORMAT_BC3_UNORM_SRGB                          = 78,
	    TEX_FORMAT_BC4_TYPELESS                            = 79,
	    TEX_FORMAT_BC4_UNORM                               = 80,
	    TEX_FORMAT_BC4_SNORM                               = 81,
	    TEX_FORMAT_BC5_TYPELESS                            = 82,
	    TEX_FORMAT_BC5_UNORM                               = 83,
	    TEX_FORMAT_BC5_SNORM                               = 84,
	    TEX_FORMAT_B5G6R5_UNORM                            = 85,
	    TEX_FORMAT_B5G5R5A1_UNORM                          = 86,
	    TEX_FORMAT_B8G8R8A8_UNORM                          = 87,
	    TEX_FORMAT_B8G8R8X8_UNORM                          = 88,
	    TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM              = 89,
	    TEX_FORMAT_B8G8R8A8_TYPELESS                       = 90,
	    TEX_FORMAT_B8G8R8A8_UNORM_SRGB                     = 91,
	    TEX_FORMAT_B8G8R8X8_TYPELESS                       = 92,
	    TEX_FORMAT_B8G8R8X8_UNORM_SRGB                     = 93,
	    TEX_FORMAT_BC6H_TYPELESS                           = 94,
	    TEX_FORMAT_BC6H_UF16                               = 95,
	    TEX_FORMAT_BC6H_SF16                               = 96,
	    TEX_FORMAT_BC7_TYPELESS                            = 97,
	    TEX_FORMAT_BC7_UNORM                               = 98,
	    TEX_FORMAT_BC7_UNORM_SRGB                          = 99,
	    TEX_FORMAT_AYUV                                    = 100,
	    TEX_FORMAT_Y410                                    = 101,
	    TEX_FORMAT_Y416                                    = 102,
	    TEX_FORMAT_NV12                                    = 103,
	    TEX_FORMAT_P010                                    = 104,
	    TEX_FORMAT_P016                                    = 105,
	    TEX_FORMAT_420_OPAQUE                              = 106,
	    TEX_FORMAT_YUY2                                    = 107,
	    TEX_FORMAT_Y210                                    = 108,
	    TEX_FORMAT_Y216                                    = 109,
	    TEX_FORMAT_NV11                                    = 110,
	    TEX_FORMAT_AI44                                    = 111,
	    TEX_FORMAT_IA44                                    = 112,
	    TEX_FORMAT_P8                                      = 113,
	    TEX_FORMAT_A8P8                                    = 114,
	    TEX_FORMAT_B4G4R4A4_UNORM                          = 115,

	    TEX_FORMAT_P208                                    = 130,
	    TEX_FORMAT_V208                                    = 131,
	    TEX_FORMAT_V408                                    = 132,

	    TEX_FORMAT_A4B4G4R4_UNORM                          = 191
	};

	enum CORE_API eSamplerFilter
    {
        SAMPLER_FILTER_MIN_MAG_MIP_POINT	= 0,
        SAMPLER_FILTER_MIN_MAG_POINT_MIP_LINEAR	= 0x1,
        SAMPLER_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT	= 0x4,
        SAMPLER_FILTER_MIN_POINT_MAG_MIP_LINEAR	= 0x5,
        SAMPLER_FILTER_MIN_LINEAR_MAG_MIP_POINT	= 0x10,
        SAMPLER_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR	= 0x11,
        SAMPLER_FILTER_MIN_MAG_LINEAR_MIP_POINT	= 0x14,
        SAMPLER_FILTER_MIN_MAG_MIP_LINEAR	= 0x15,
        SAMPLER_FILTER_MIN_MAG_ANISOTROPIC_MIP_POINT	= 0x54,
        SAMPLER_FILTER_ANISOTROPIC	= 0x55,
        SAMPLER_FILTER_COMPARISON_MIN_MAG_MIP_POINT	= 0x80,
        SAMPLER_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR	= 0x81,
        SAMPLER_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT	= 0x84,
        SAMPLER_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR	= 0x85,
        SAMPLER_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT	= 0x90,
        SAMPLER_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR	= 0x91,
        SAMPLER_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT	= 0x94,
        SAMPLER_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR	= 0x95,
        SAMPLER_FILTER_COMPARISON_MIN_MAG_ANISOTROPIC_MIP_POINT	= 0xd4,
        SAMPLER_FILTER_COMPARISON_ANISOTROPIC	= 0xd5,
        SAMPLER_FILTER_MINIMUM_MIN_MAG_MIP_POINT	= 0x100,
        SAMPLER_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR	= 0x101,
        SAMPLER_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT	= 0x104,
        SAMPLER_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR	= 0x105,
        SAMPLER_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT	= 0x110,
        SAMPLER_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR	= 0x111,
        SAMPLER_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT	= 0x114,
        SAMPLER_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR	= 0x115,
        SAMPLER_FILTER_MINIMUM_MIN_MAG_ANISOTROPIC_MIP_POINT	= 0x154,
        SAMPLER_FILTER_MINIMUM_ANISOTROPIC	= 0x155,
        SAMPLER_FILTER_MAXIMUM_MIN_MAG_MIP_POINT	= 0x180,
        SAMPLER_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR	= 0x181,
        SAMPLER_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT	= 0x184,
        SAMPLER_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR	= 0x185,
        SAMPLER_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT	= 0x190,
        SAMPLER_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR	= 0x191,
        SAMPLER_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT	= 0x194,
        SAMPLER_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR	= 0x195,
        SAMPLER_FILTER_MAXIMUM_MIN_MAG_ANISOTROPIC_MIP_POINT	= 0x1d4,
        SAMPLER_FILTER_MAXIMUM_ANISOTROPIC	= 0x1d5
    };

	enum CORE_API ePrimitiveTopologyType
    {
        PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED	= 0,
        PRIMITIVE_TOPOLOGY_TYPE_POINT	= 1,
        PRIMITIVE_TOPOLOGY_TYPE_LINE	= 2,
        PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE	= 3,
        PRIMITIVE_TOPOLOGY_TYPE_PATCH	= 4
    };

	enum CORE_API ePrimitiveTopology
    {
        PRIMITIVE_TOPOLOGY_UNDEFINED	= 0,
        PRIMITIVE_TOPOLOGY_POINTLIST	= 1,
        PRIMITIVE_TOPOLOGY_LINELIST	= 2,
        PRIMITIVE_TOPOLOGY_LINESTRIP	= 3,
        PRIMITIVE_TOPOLOGY_TRIANGLELIST	= 4,
        PRIMITIVE_TOPOLOGY_TRIANGLESTRIP	= 5,
        PRIMITIVE_TOPOLOGY_TRIANGLEFAN	= 6,
        PRIMITIVE_TOPOLOGY_LINELIST_ADJ	= 10,
        PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ	= 11,
        PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ	= 12,
        PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ	= 13,
        PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST	= 33,
        PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST	= 34,
        PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST	= 35,
        PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST	= 36,
        PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST	= 37,
        PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST	= 38,
        PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST	= 39,
        PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST	= 40,
        PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST	= 41,
        PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST	= 42,
        PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST	= 43,
        PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST	= 44,
        PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST	= 45,
        PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST	= 46,
        PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST	= 47,
        PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST	= 48,
        PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST	= 49,
        PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST	= 50,
        PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST	= 51,
        PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST	= 52,
        PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST	= 53,
        PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST	= 54,
        PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST	= 55,
        PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST	= 56,
        PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST	= 57,
        PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST	= 58,
        PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST	= 59,
        PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST	= 60,
        PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST	= 61,
        PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST	= 62,
        PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST	= 63,
        PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST	= 64
    };

	enum CORE_API eResourceFlag
	{
		RESOURCE_FLAG_NONE = 0,
		RESOURCE_FLAG_ALLOW_RENDER_TARGET = 0x1,
		RESOURCE_FLAG_ALLOW_DEPTH_STENCIL = 0x2,
		RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS = 0x4,
		RESOURCE_FLAG_DENY_SHADER_RESOURCE = 0x8,
		RESOURCE_FLAG_ALLOW_CROSS_ADAPTER = 0x10,
		RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS = 0x20,
		RESOURCE_FLAG_VIDEO_DECODE_REFERENCE_ONLY = 0x40,
		RESOURCE_FLAG_VIDEO_ENCODE_REFERENCE_ONLY = 0x80,
		RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE = 0x100
	};

	using eResourceFlags = UINT;

	enum CORE_API eTextureLayout : uint8_t
	{
		TEX_LAYOUT_UNKNOWN = 0,
		TEX_LAYOUT_ROW_MAJOR = 1,
		TEX_LAYOUT_64KB_UNDEFINED_SWIZZLE = 2,
		TEX_LAYOUT_64KB_STANDARD_SWIZZLE = 3
	};

	struct SamplerDescription
	{
		UINT Count;
		UINT Quality;
	};

	enum CORE_API eTexType
	{
		TEX_TYPE_UNKNOWN = -1,
		TEX_TYPE_1D,
		TEX_TYPE_2D,
		TEX_TYPE_3D,
		TEX_TYPE_BUFFER,
	};

	enum CORE_API eUAVNativeType
	{
		UAV_DIMENSION_UNKNOWN	= 0,
		UAV_DIMENSION_BUFFER	= 1,
		UAV_DIMENSION_TEXTURE1D	= 2,
		UAV_DIMENSION_TEXTURE1DARRAY	= 3,
		UAV_DIMENSION_TEXTURE2D	= 4,
		UAV_DIMENSION_TEXTURE2DARRAY	= 5,
		UAV_DIMENSION_TEXTURE2DMS	= 6,
		UAV_DIMENSION_TEXTURE2DMSARRAY	= 7,
		UAV_DIMENSION_TEXTURE3D	= 8
	};

	enum CORE_API eUAVBufferFlag
	{
		BUFFER_UAV_FLAG_NONE	= 0,
		BUFFER_UAV_FLAG_RAW	= 0x1
	};

	struct CORE_API BufferUAVDescription
	{
		UINT64 FirstElement;
		UINT NumElements;
		UINT StructureByteStride;
		UINT64 CounterOffsetInBytes;
		eUAVBufferFlag Flags;
	};

	struct CORE_API Tex1dUAVDescription
	{
		UINT MipSlice;
	};

	struct CORE_API Tex1dArrayUAVDescription
	{
		UINT MipSlice;
		UINT FirstArraySlice;
		UINT ArraySize;
	};

	struct CORE_API Tex2dUAVDescription
	{
		UINT MipSlice;
		UINT PlaneSlice;
	};

	struct CORE_API Tex2dArrayUAVDescription
	{
		UINT MipSlice;
		UINT FirstArraySlice;
		UINT ArraySize;
		UINT PlaneSlice;
	};

	struct CORE_API Tex2dMsUAVDescription
	{
		UINT UnusedField_NothingToDefine;
	};

	struct CORE_API Tex2dMsArrayUAVDescription
	{
		UINT FirstArraySlice;
		UINT ArraySize;
	};

	struct CORE_API Tex3dUAVDescription
	{
		UINT MipSlice;
		UINT FirstWSlice;
		UINT WSize;
	};

	struct CORE_API UAVDescription
	{
		eFormat Format;
		eUAVNativeType ViewDimension;
		union 
		{
			BufferUAVDescription Buffer;
			Tex1dUAVDescription Texture1D;
			Tex1dArrayUAVDescription Texture1DArray;
			Tex2dUAVDescription Texture2D;
			Tex2dArrayUAVDescription Texture2DArray;
			Tex2dMsUAVDescription Texture2DMS;
			Tex2dMsArrayUAVDescription Texture2DMSArray;
			Tex3dUAVDescription Texture3D;
		};
	};

	struct CORE_API BufferRtvDescription
	{
		UINT64 FirstElement;
		UINT NumElements;
	};

	struct CORE_API Tex1dRtvDescription
	{
		UINT MipSlice;
	};

	struct CORE_API Tex1dArrayRtvDescription
	{
		UINT MipSlice;
		UINT FirstArraySlice;
		UINT ArraySize;
	};

	struct CORE_API Tex2dRtvDescription
	{
		UINT MipSlice;
		UINT PlaneSlice;
	};

	struct CORE_API Tex2dMsRtvDescription
	{
		UINT UnusedField_NothingToDefine;
	};

	struct CORE_API Tex2dArrayRtvDescription
	{
		UINT MipSlice;
		UINT FirstArraySlice;
		UINT ArraySize;
		UINT PlaneSlice;
	};

	struct CORE_API Tex2dMsArrayRtvDescription
	{
		UINT FirstArraySlice;
		UINT ArraySize;
	};

	struct CORE_API Tex3dRtvDescription
	{
		UINT MipSlice;
		UINT FirstWSlice;
		UINT WSize;
	};

	enum CORE_API eNativeRtvType
	{
		RTV_DIMENSION_UNKNOWN	= 0,
		RTV_DIMENSION_BUFFER	= 1,
		RTV_DIMENSION_TEXTURE1D	= 2,
		RTV_DIMENSION_TEXTURE1DARRAY	= 3,
		RTV_DIMENSION_TEXTURE2D	= 4,
		RTV_DIMENSION_TEXTURE2DARRAY	= 5,
		RTV_DIMENSION_TEXTURE2DMS	= 6,
		RTV_DIMENSION_TEXTURE2DMSARRAY	= 7,
		RTV_DIMENSION_TEXTURE3D	= 8
	};

	struct CORE_API RtvDescription
	{
		eFormat Format;
		eNativeRtvType ViewDimension;
		union 
		{
			BufferRtvDescription Buffer;
			Tex1dRtvDescription Texture1D;
			Tex1dArrayRtvDescription Texture1DArray;
			Tex2dRtvDescription Texture2D;
			Tex2dArrayRtvDescription Texture2DArray;
			Tex2dMsRtvDescription Texture2DMS;
			Tex2dMsArrayRtvDescription Texture2DMSArray;
			Tex3dRtvDescription Texture3D;
		};
	};

	struct CORE_API Tex1dDsvDescription
	{
		UINT MipSlice;
	};

	struct CORE_API Tex1dArrayDsvDescription
	{
		UINT MipSlice;
		UINT FirstArraySlice;
		UINT ArraySize;
	};

	struct CORE_API Tex2dDsvDescription
	{
		UINT MipSlice;
	};

	struct CORE_API Tex2dArrayDsvDescription
	{
		UINT MipSlice;
		UINT FirstArraySlice;
		UINT ArraySize;
	};

	struct CORE_API Tex2dMsDsvDescription
	{
		UINT UnusedField_NothingToDefine;
	};

	struct CORE_API Tex2dMsArrayDsvDescription
	{
		UINT FirstArraySlice;
		UINT ArraySize;
	};

	
	enum CORE_API eDsvFlag
	{
		D3D12_DSV_FLAG_NONE	= 0,
		D3D12_DSV_FLAG_READ_ONLY_DEPTH	= 0x1,
		D3D12_DSV_FLAG_READ_ONLY_STENCIL	= 0x2
	};
	
	enum CORE_API eNativeDsvType
	{
		DSV_DIMENSION_UNKNOWN	= 0,
		DSV_DIMENSION_TEXTURE1D	= 1,
		DSV_DIMENSION_TEXTURE1DARRAY	= 2,
		DSV_DIMENSION_TEXTURE2D	= 3,
		DSV_DIMENSION_TEXTURE2DARRAY	= 4,
		DSV_DIMENSION_TEXTURE2DMS	= 5,
		DSV_DIMENSION_TEXTURE2DMSARRAY	= 6
	};

	struct DsvDescription
	{
		eFormat Format;
		eNativeDsvType ViewDimension;
		eDsvFlag Flags;
		union
		{
			Tex1dDsvDescription Texture1D;
			Tex1dArrayDsvDescription Texture1DArray;
			Tex2dDsvDescription Texture2D;
			Tex2dArrayDsvDescription Texture2DArray;
			Tex2dMsDsvDescription Texture2DMS;
			Tex2dMsArrayDsvDescription Texture2DMSArray;
		};
	};
	
	enum eSrvFlag
    {
        BUFFER_SRV_FLAG_NONE	= 0,
        BUFFER_SRV_FLAG_RAW	= 0x1
    };

	struct BufferSrvDescription
    {
		UINT64 FirstElement;
		UINT NumElements;
		UINT StructureByteStride;
		eSrvFlag Flags;
    };

	struct Tex1dSrvDescription
	{
	    UINT MostDetailedMip;
	    UINT MipLevels;
	    FLOAT ResourceMinLODClamp;
    };

	struct Tex1dArraySrvDescription
    {
		UINT MostDetailedMip;
		UINT MipLevels;
		UINT FirstArraySlice;
		UINT ArraySize;
		FLOAT ResourceMinLODClamp;
    };

	struct Tex2dSrvDescription
    {
		UINT MostDetailedMip;
		UINT MipLevels;
		UINT PlaneSlice;
		FLOAT ResourceMinLODClamp;
    };

	struct Tex2dArraySrvDescription
    {
	    UINT MostDetailedMip;
	    UINT MipLevels;
	    UINT FirstArraySlice;
	    UINT ArraySize;
	    UINT PlaneSlice;
	    FLOAT ResourceMinLODClamp;
    };

	struct Tex3dSrvDescription
    {
		UINT MostDetailedMip;
		UINT MipLevels;
		FLOAT ResourceMinLODClamp;
    };

	struct TexCubeSrvDescription
    {
	    UINT MostDetailedMip;
	    UINT MipLevels;
	    FLOAT ResourceMinLODClamp;
	};

	struct TexCubeArraySrvDescription
    {
		UINT MostDetailedMip;
		UINT MipLevels;
		UINT First2DArrayFace;
		UINT NumCubes;
		FLOAT ResourceMinLODClamp;
    };

	struct Tex2dMsSrvDescription
    {
		UINT UnusedField_NothingToDefine;
    };

	struct Tex2dMsArraySrvDescription
	{
	    UINT FirstArraySlice;
	    UINT ArraySize;
    };

	struct AccelStructSrvDescription
    {
		uint64_t Location; //todo: address type;
    };
 
	enum eNativeSrvType
    {
        SRV_DIMENSION_UNKNOWN	= 0,
        SRV_DIMENSION_BUFFER	= 1,
        SRV_DIMENSION_TEXTURE1D	= 2,
        SRV_DIMENSION_TEXTURE1DARRAY	= 3,
        SRV_DIMENSION_TEXTURE2D	= 4,
        SRV_DIMENSION_TEXTURE2DARRAY	= 5,
        SRV_DIMENSION_TEXTURE2DMS	= 6,
        SRV_DIMENSION_TEXTURE2DMSARRAY	= 7,
        SRV_DIMENSION_TEXTURE3D	= 8,
        SRV_DIMENSION_TEXTURECUBE	= 9,
        SRV_DIMENSION_TEXTURECUBEARRAY	= 10,
        SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE	= 11
    };

	struct SrvDescription
    {
	    eFormat Format;
	    eNativeSrvType ViewDimension;
	    UINT Shader4ComponentMapping;
	    union 
	        {
		        BufferSrvDescription Buffer;
		        Tex1dSrvDescription Texture1D;
		        Tex1dArraySrvDescription Texture1DArray;
		        Tex2dSrvDescription Texture2D;
		        Tex2dArraySrvDescription Texture2DArray;
		        Tex2dMsSrvDescription Texture2DMS;
		        Tex2dMsArraySrvDescription Texture2DMSArray;
		        Tex3dSrvDescription Texture3D;
		        TexCubeSrvDescription TextureCube;
		        TexCubeArraySrvDescription TextureCubeArray;
		        AccelStructSrvDescription RaytracingAccelerationStructure;
	        };
    };

	struct CORE_API GenericTextureDescription
	{
		eTexType				 Dimension		  = TEX_TYPE_UNKNOWN;
		UINT64                   Alignment        = 0;
		UINT64                   Width            = 0;
		UINT                     Height           = 0;
		UINT16                   DepthOrArraySize = 0;
		eFormat				     Format           = TEX_FORMAT_R32G32B32A32_FLOAT;
		eResourceFlags			 Flags            = RESOURCE_FLAG_NONE;
		UINT16                   MipsLevel        = 1;
		eTextureLayout			 Layout           = TEX_LAYOUT_64KB_STANDARD_SWIZZLE;
		SamplerDescription       SampleDesc       = {.Count = 1, .Quality = 0};
		bool					 AsSRV	  = true;
		bool					 AsRTV	  = false;
		bool					 AsDSV	  = false;
		bool					 AsUAV	  = false;
		SrvDescription			 Srv{};
		RtvDescription			 Rtv{};
		DsvDescription			 Dsv{};
		UAVDescription			 Uav{};
	};

	struct CORE_API Viewport
	{
		float topLeftX;
		float topLeftY;
		float width;
		float height;
		float minDepth;
		float maxDepth;
	};

	struct CORE_API PrimitivePipeline
	{
	public:
		virtual      ~PrimitivePipeline() = default;
		virtual void Generate() = 0;
		[[nodiscard]] void* GetNativePipeline() const;

	protected:
		void SetPrimitivePipeline(void* pipeline);

	private:
		void* m_pipeline_ = nullptr;
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

	using ObjectPredication = std::function<bool(const Strong<Abstracts::ObjectBase>&)>;
}
