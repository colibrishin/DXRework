#pragma once
#include "ConstantBuffer.h"
#include "StructuredBuffer.h"
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

namespace Engine
{
	enum CORE_API eHeapType
	{
		HEAP_TYPE_DEFAULT	= 1,
		HEAP_TYPE_UPLOAD	= 2,
		HEAP_TYPE_READBACK	= 3,
		HEAP_TYPE_CUSTOM	= 4,
		HEAP_TYPE_GPU_UPLOAD	= 5
	};

	enum CORE_API eHeapFlag
	{
		HEAP_FLAG_NONE	= 0,
		HEAP_FLAG_SHARED	= 0x1,
		HEAP_FLAG_DENY_BUFFERS	= 0x4,
		HEAP_FLAG_ALLOW_DISPLAY	= 0x8,
		HEAP_FLAG_SHARED_CROSS_ADAPTER	= 0x20,
		HEAP_FLAG_DENY_RT_DS_TEXTURES	= 0x40,
		HEAP_FLAG_DENY_NON_RT_DS_TEXTURES	= 0x80,
		HEAP_FLAG_HARDWARE_PROTECTED	= 0x100,
		HEAP_FLAG_ALLOW_WRITE_WATCH	= 0x200,
		HEAP_FLAG_ALLOW_SHADER_ATOMICS	= 0x400,
		HEAP_FLAG_CREATE_NOT_RESIDENT	= 0x800,
		HEAP_FLAG_CREATE_NOT_ZEROED	= 0x1000,
		HEAP_FLAG_TOOLS_USE_MANUAL_WRITE_TRACKING	= 0x2000,
		HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES	= 0,
		HEAP_FLAG_ALLOW_ONLY_BUFFERS	= 0xc0,
		HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES	= 0x44,
		HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES	= 0x84
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

	enum CORE_API eFormat : uint8_t
	{
		TEX_FORMAT_UNKNOWN = 0,
		TEX_FORMAT_R32G32B32A32_TYPELESS = 1,
		TEX_FORMAT_R32G32B32A32_FLOAT = 2,
		TEX_FORMAT_R32G32B32A32_UINT = 3,
		TEX_FORMAT_R32G32B32A32_SINT = 4,
		TEX_FORMAT_R32G32B32_TYPELESS = 5,
		TEX_FORMAT_R32G32B32_FLOAT = 6,
		TEX_FORMAT_R32G32B32_UINT = 7,
		TEX_FORMAT_R32G32B32_SINT = 8,
		TEX_FORMAT_R16G16B16A16_TYPELESS = 9,
		TEX_FORMAT_R16G16B16A16_FLOAT = 10,
		TEX_FORMAT_R16G16B16A16_UNORM = 11,
		TEX_FORMAT_R16G16B16A16_UINT = 12,
		TEX_FORMAT_R16G16B16A16_SNORM = 13,
		TEX_FORMAT_R16G16B16A16_SINT = 14,
		TEX_FORMAT_R32G32_TYPELESS = 15,
		TEX_FORMAT_R32G32_FLOAT = 16,
		TEX_FORMAT_R32G32_UINT = 17,
		TEX_FORMAT_R32G32_SINT = 18,
		TEX_FORMAT_R32G8X24_TYPELESS = 19,
		TEX_FORMAT_D32_FLOAT_S8X24_UINT = 20,
		TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
		TEX_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
		TEX_FORMAT_R10G10B10A2_TYPELESS = 23,
		TEX_FORMAT_R10G10B10A2_UNORM = 24,
		TEX_FORMAT_R10G10B10A2_UINT = 25,
		TEX_FORMAT_R11G11B10_FLOAT = 26,
		TEX_FORMAT_R8G8B8A8_TYPELESS = 27,
		TEX_FORMAT_R8G8B8A8_UNORM = 28,
		TEX_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
		TEX_FORMAT_R8G8B8A8_UINT = 30,
		TEX_FORMAT_R8G8B8A8_SNORM = 31,
		TEX_FORMAT_R8G8B8A8_SINT = 32,
		TEX_FORMAT_R16G16_TYPELESS = 33,
		TEX_FORMAT_R16G16_FLOAT = 34,
		TEX_FORMAT_R16G16_UNORM = 35,
		TEX_FORMAT_R16G16_UINT = 36,
		TEX_FORMAT_R16G16_SNORM = 37,
		TEX_FORMAT_R16G16_SINT = 38,
		TEX_FORMAT_R32_TYPELESS = 39,
		TEX_FORMAT_D32_FLOAT = 40,
		TEX_FORMAT_R32_FLOAT = 41,
		TEX_FORMAT_R32_UINT = 42,
		TEX_FORMAT_R32_SINT = 43,
		TEX_FORMAT_R24G8_TYPELESS = 44,
		TEX_FORMAT_D24_UNORM_S8_UINT = 45,
		TEX_FORMAT_R24_UNORM_X8_TYPELESS = 46,
		TEX_FORMAT_X24_TYPELESS_G8_UINT = 47,
		TEX_FORMAT_R8G8_TYPELESS = 48,
		TEX_FORMAT_R8G8_UNORM = 49,
		TEX_FORMAT_R8G8_UINT = 50,
		TEX_FORMAT_R8G8_SNORM = 51,
		TEX_FORMAT_R8G8_SINT = 52,
		TEX_FORMAT_R16_TYPELESS = 53,
		TEX_FORMAT_R16_FLOAT = 54,
		TEX_FORMAT_D16_UNORM = 55,
		TEX_FORMAT_R16_UNORM = 56,
		TEX_FORMAT_R16_UINT = 57,
		TEX_FORMAT_R16_SNORM = 58,
		TEX_FORMAT_R16_SINT = 59,
		TEX_FORMAT_R8_TYPELESS = 60,
		TEX_FORMAT_R8_UNORM = 61,
		TEX_FORMAT_R8_UINT = 62,
		TEX_FORMAT_R8_SNORM = 63,
		TEX_FORMAT_R8_SINT = 64,
		TEX_FORMAT_A8_UNORM = 65,
		TEX_FORMAT_R1_UNORM = 66,
		TEX_FORMAT_R9G9B9E5_SHAREDEXP = 67,
		TEX_FORMAT_R8G8_B8G8_UNORM = 68,
		TEX_FORMAT_G8R8_G8B8_UNORM = 69,
		TEX_FORMAT_BC1_TYPELESS = 70,
		TEX_FORMAT_BC1_UNORM = 71,
		TEX_FORMAT_BC1_UNORM_SRGB = 72,
		TEX_FORMAT_BC2_TYPELESS = 73,
		TEX_FORMAT_BC2_UNORM = 74,
		TEX_FORMAT_BC2_UNORM_SRGB = 75,
		TEX_FORMAT_BC3_TYPELESS = 76,
		TEX_FORMAT_BC3_UNORM = 77,
		TEX_FORMAT_BC3_UNORM_SRGB = 78,
		TEX_FORMAT_BC4_TYPELESS = 79,
		TEX_FORMAT_BC4_UNORM = 80,
		TEX_FORMAT_BC4_SNORM = 81,
		TEX_FORMAT_BC5_TYPELESS = 82,
		TEX_FORMAT_BC5_UNORM = 83,
		TEX_FORMAT_BC5_SNORM = 84,
		TEX_FORMAT_B5G6R5_UNORM = 85,
		TEX_FORMAT_B5G5R5A1_UNORM = 86,
		TEX_FORMAT_B8G8R8A8_UNORM = 87,
		TEX_FORMAT_B8G8R8X8_UNORM = 88,
		TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
		TEX_FORMAT_B8G8R8A8_TYPELESS = 90,
		TEX_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
		TEX_FORMAT_B8G8R8X8_TYPELESS = 92,
		TEX_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
		TEX_FORMAT_BC6H_TYPELESS = 94,
		TEX_FORMAT_BC6H_UF16 = 95,
		TEX_FORMAT_BC6H_SF16 = 96,
		TEX_FORMAT_BC7_TYPELESS = 97,
		TEX_FORMAT_BC7_UNORM = 98,
		TEX_FORMAT_BC7_UNORM_SRGB = 99,
		TEX_FORMAT_AYUV = 100,
		TEX_FORMAT_Y410 = 101,
		TEX_FORMAT_Y416 = 102,
		TEX_FORMAT_NV12 = 103,
		TEX_FORMAT_P010 = 104,
		TEX_FORMAT_P016 = 105,
		TEX_FORMAT_420_OPAQUE = 106,
		TEX_FORMAT_YUY2 = 107,
		TEX_FORMAT_Y210 = 108,
		TEX_FORMAT_Y216 = 109,
		TEX_FORMAT_NV11 = 110,
		TEX_FORMAT_AI44 = 111,
		TEX_FORMAT_IA44 = 112,
		TEX_FORMAT_P8 = 113,
		TEX_FORMAT_A8P8 = 114,
		TEX_FORMAT_B4G4R4A4_UNORM = 115,

		TEX_FORMAT_P208 = 130,
		TEX_FORMAT_V208 = 131,
		TEX_FORMAT_V408 = 132,

		TEX_FORMAT_A4B4G4R4_UNORM = 191
	};

	constexpr std::vector<eFormat> GetDefaultRTVFormat() 
	{
		std::vector<eFormat> v = { TEX_FORMAT_R8G8B8A8_UNORM };
		return v;
	}

	enum CORE_API eSamplerFilter
	{
		SAMPLER_FILTER_MIN_MAG_MIP_POINT = 0,
		SAMPLER_FILTER_MIN_MAG_POINT_MIP_LINEAR = 0x1,
		SAMPLER_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x4,
		SAMPLER_FILTER_MIN_POINT_MAG_MIP_LINEAR = 0x5,
		SAMPLER_FILTER_MIN_LINEAR_MAG_MIP_POINT = 0x10,
		SAMPLER_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x11,
		SAMPLER_FILTER_MIN_MAG_LINEAR_MIP_POINT = 0x14,
		SAMPLER_FILTER_MIN_MAG_MIP_LINEAR = 0x15,
		SAMPLER_FILTER_MIN_MAG_ANISOTROPIC_MIP_POINT = 0x54,
		SAMPLER_FILTER_ANISOTROPIC = 0x55,
		SAMPLER_FILTER_COMPARISON_MIN_MAG_MIP_POINT = 0x80,
		SAMPLER_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR = 0x81,
		SAMPLER_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x84,
		SAMPLER_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR = 0x85,
		SAMPLER_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT = 0x90,
		SAMPLER_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x91,
		SAMPLER_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT = 0x94,
		SAMPLER_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR = 0x95,
		SAMPLER_FILTER_COMPARISON_MIN_MAG_ANISOTROPIC_MIP_POINT = 0xd4,
		SAMPLER_FILTER_COMPARISON_ANISOTROPIC = 0xd5,
		SAMPLER_FILTER_MINIMUM_MIN_MAG_MIP_POINT = 0x100,
		SAMPLER_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR = 0x101,
		SAMPLER_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x104,
		SAMPLER_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR = 0x105,
		SAMPLER_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT = 0x110,
		SAMPLER_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x111,
		SAMPLER_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT = 0x114,
		SAMPLER_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR = 0x115,
		SAMPLER_FILTER_MINIMUM_MIN_MAG_ANISOTROPIC_MIP_POINT = 0x154,
		SAMPLER_FILTER_MINIMUM_ANISOTROPIC = 0x155,
		SAMPLER_FILTER_MAXIMUM_MIN_MAG_MIP_POINT = 0x180,
		SAMPLER_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR = 0x181,
		SAMPLER_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x184,
		SAMPLER_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR = 0x185,
		SAMPLER_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT = 0x190,
		SAMPLER_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x191,
		SAMPLER_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT = 0x194,
		SAMPLER_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR = 0x195,
		SAMPLER_FILTER_MAXIMUM_MIN_MAG_ANISOTROPIC_MIP_POINT = 0x1d4,
		SAMPLER_FILTER_MAXIMUM_ANISOTROPIC = 0x1d5
	};

	enum CORE_API ePrimitiveTopologyType
	{
		PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED = 0,
		PRIMITIVE_TOPOLOGY_TYPE_POINT = 1,
		PRIMITIVE_TOPOLOGY_TYPE_LINE = 2,
		PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE = 3,
		PRIMITIVE_TOPOLOGY_TYPE_PATCH = 4
	};

	enum CORE_API ePrimitiveTopology
	{
		PRIMITIVE_TOPOLOGY_UNDEFINED = 0,
		PRIMITIVE_TOPOLOGY_POINTLIST = 1,
		PRIMITIVE_TOPOLOGY_LINELIST = 2,
		PRIMITIVE_TOPOLOGY_LINESTRIP = 3,
		PRIMITIVE_TOPOLOGY_TRIANGLELIST = 4,
		PRIMITIVE_TOPOLOGY_TRIANGLESTRIP = 5,
		PRIMITIVE_TOPOLOGY_TRIANGLEFAN = 6,
		PRIMITIVE_TOPOLOGY_LINELIST_ADJ = 10,
		PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ = 11,
		PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ = 12,
		PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ = 13,
		PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST = 33,
		PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST = 34,
		PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST = 35,
		PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST = 36,
		PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST = 37,
		PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST = 38,
		PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST = 39,
		PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST = 40,
		PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST = 41,
		PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST = 42,
		PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST = 43,
		PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST = 44,
		PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST = 45,
		PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST = 46,
		PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST = 47,
		PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST = 48,
		PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST = 49,
		PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST = 50,
		PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST = 51,
		PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST = 52,
		PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST = 53,
		PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST = 54,
		PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST = 55,
		PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST = 56,
		PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST = 57,
		PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST = 58,
		PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST = 59,
		PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST = 60,
		PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST = 61,
		PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST = 62,
		PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST = 63,
		PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST = 64
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
		UAV_DIMENSION_UNKNOWN = 0,
		UAV_DIMENSION_BUFFER = 1,
		UAV_DIMENSION_TEXTURE1D = 2,
		UAV_DIMENSION_TEXTURE1DARRAY = 3,
		UAV_DIMENSION_TEXTURE2D = 4,
		UAV_DIMENSION_TEXTURE2DARRAY = 5,
		UAV_DIMENSION_TEXTURE2DMS = 6,
		UAV_DIMENSION_TEXTURE2DMSARRAY = 7,
		UAV_DIMENSION_TEXTURE3D = 8
	};

	enum CORE_API eUAVBufferFlag
	{
		BUFFER_UAV_FLAG_NONE = 0,
		BUFFER_UAV_FLAG_RAW = 0x1
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
		RTV_DIMENSION_UNKNOWN = 0,
		RTV_DIMENSION_BUFFER = 1,
		RTV_DIMENSION_TEXTURE1D = 2,
		RTV_DIMENSION_TEXTURE1DARRAY = 3,
		RTV_DIMENSION_TEXTURE2D = 4,
		RTV_DIMENSION_TEXTURE2DARRAY = 5,
		RTV_DIMENSION_TEXTURE2DMS = 6,
		RTV_DIMENSION_TEXTURE2DMSARRAY = 7,
		RTV_DIMENSION_TEXTURE3D = 8
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
		DSV_FLAG_NONE = 0,
		DSV_FLAG_READ_ONLY_DEPTH = 0x1,
		DSV_FLAG_READ_ONLY_STENCIL = 0x2
	};

	enum CORE_API eNativeDsvType
	{
		DSV_DIMENSION_UNKNOWN = 0,
		DSV_DIMENSION_TEXTURE1D = 1,
		DSV_DIMENSION_TEXTURE1DARRAY = 2,
		DSV_DIMENSION_TEXTURE2D = 3,
		DSV_DIMENSION_TEXTURE2DARRAY = 4,
		DSV_DIMENSION_TEXTURE2DMS = 5,
		DSV_DIMENSION_TEXTURE2DMSARRAY = 6
	};

	struct CORE_API DsvDescription
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

	enum CORE_API eSrvFlag
	{
		BUFFER_SRV_FLAG_NONE = 0,
		BUFFER_SRV_FLAG_RAW = 0x1
	};

	struct CORE_API BufferSrvDescription
	{
		UINT64 FirstElement;
		UINT NumElements;
		UINT StructureByteStride;
		eSrvFlag Flags;
	};

	struct CORE_API Tex1dSrvDescription
	{
		UINT MostDetailedMip;
		UINT MipLevels;
		FLOAT ResourceMinLODClamp;
	};

	struct CORE_API Tex1dArraySrvDescription
	{
		UINT MostDetailedMip;
		UINT MipLevels;
		UINT FirstArraySlice;
		UINT ArraySize;
		FLOAT ResourceMinLODClamp;
	};

	struct CORE_API Tex2dSrvDescription
	{
		UINT MostDetailedMip;
		UINT MipLevels;
		UINT PlaneSlice;
		FLOAT ResourceMinLODClamp;
	};

	struct CORE_API Tex2dArraySrvDescription
	{
		UINT MostDetailedMip;
		UINT MipLevels;
		UINT FirstArraySlice;
		UINT ArraySize;
		UINT PlaneSlice;
		FLOAT ResourceMinLODClamp;
	};

	struct CORE_API Tex3dSrvDescription
	{
		UINT MostDetailedMip;
		UINT MipLevels;
		FLOAT ResourceMinLODClamp;
	};

	struct CORE_API TexCubeSrvDescription
	{
		UINT MostDetailedMip;
		UINT MipLevels;
		FLOAT ResourceMinLODClamp;
	};

	struct CORE_API TexCubeArraySrvDescription
	{
		UINT MostDetailedMip;
		UINT MipLevels;
		UINT First2DArrayFace;
		UINT NumCubes;
		FLOAT ResourceMinLODClamp;
	};

	struct CORE_API Tex2dMsSrvDescription
	{
		UINT UnusedField_NothingToDefine;
	};

	struct CORE_API Tex2dMsArraySrvDescription
	{
		UINT FirstArraySlice;
		UINT ArraySize;
	};

	struct CORE_API AccelStructSrvDescription
	{
		uint64_t Location; //todo: address type;
	};

	enum CORE_API eNativeSrvType
	{
		SRV_DIMENSION_UNKNOWN = 0,
		SRV_DIMENSION_BUFFER = 1,
		SRV_DIMENSION_TEXTURE1D = 2,
		SRV_DIMENSION_TEXTURE1DARRAY = 3,
		SRV_DIMENSION_TEXTURE2D = 4,
		SRV_DIMENSION_TEXTURE2DARRAY = 5,
		SRV_DIMENSION_TEXTURE2DMS = 6,
		SRV_DIMENSION_TEXTURE2DMSARRAY = 7,
		SRV_DIMENSION_TEXTURE3D = 8,
		SRV_DIMENSION_TEXTURECUBE = 9,
		SRV_DIMENSION_TEXTURECUBEARRAY = 10,
		SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE = 11
	};

	struct CORE_API SrvDescription
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
		eTexType				 Dimension = TEX_TYPE_UNKNOWN;
		UINT64                   Alignment = 0;
		UINT64                   Width = 0;
		UINT                     Height = 0;
		UINT16                   DepthOrArraySize = 0;
		eFormat				     Format = TEX_FORMAT_R32G32B32A32_FLOAT;
		eResourceFlags			 Flags = RESOURCE_FLAG_NONE;
		UINT16                   MipsLevel = 1;
		eTextureLayout			 Layout = TEX_LAYOUT_64KB_STANDARD_SWIZZLE;
		SamplerDescription       SampleDesc = { .Count = 1, .Quality = 0 };
		bool					 AsSRV = true;
		bool					 AsRTV = false;
		bool					 AsDSV = false;
		bool					 AsUAV = false;
		SrvDescription			 Srv{};
		RtvDescription			 Rtv{};
		DsvDescription			 Dsv{};
		UAVDescription			 Uav{};
	};

	struct CORE_API PrimitiveTexture
	{
		virtual      ~PrimitiveTexture() = default;
		virtual void Generate(Resources::Texture* texture) = 0;
		virtual void LoadFromFile(Resources::Texture* texture, const std::filesystem::path& path) = 0;
		virtual void SaveAsFile(const std::filesystem::path& path) = 0;

		virtual void Map(
			void* data_ptr, 
			const size_t width,
			const size_t height,
			const size_t stride,
			const size_t depth) = 0;
		
		virtual void Map( 
			PrimitiveTexture* src,
			const UINT src_width,
			const UINT src_height,
			const size_t src_idx,
			const UINT dst_x,
			const UINT dst_y,
			const size_t dst_idx) = 0;

		void UpdateDescription(const GenericTextureDescription& description)
		{
			m_description_ = description;
		}

		[[nodiscard]] void* GetNativeTexture() const
		{
			return m_texture_;
		}

		[[nodiscard]] const GenericTextureDescription& GetDescription() const
		{
			return m_description_;
		}

	protected:
		virtual void SetPrimitiveTexture(void* texture)
		{
			m_texture_ = texture;
		}

	private:
		GenericTextureDescription m_description_;
		void* m_texture_ = nullptr;
	};

	struct CORE_API GraphicPrimitiveShader
	{
	public:
		virtual             ~GraphicPrimitiveShader() = default;
		virtual void        Generate(const Resources::Shader* shader, void* pipeline_signature) = 0;
		[[nodiscard]] void* GetNativeShader() const
		{
			return m_shader_;
		}
		[[nodiscard]] void* GetNativeSampler() const 
		{
			return m_sampler_;
		}

	protected:
		virtual void SetNativeShader(void* shader) 
		{
			m_shader_ = shader;
		}

		virtual void SetNativeSampler(void* sampler) 
		{
			m_sampler_ = sampler;
		}

	private:
		void* m_shader_ = nullptr;
		void* m_sampler_ = nullptr;
	};

	struct CORE_API ComputePrimitiveShader
	{
	public:
		virtual      ~ComputePrimitiveShader() = default;
		virtual void Generate(Resources::ComputeShader* shader, void* pipeline_signature) = 0;

		[[nodiscard]] void* GetNativeShader() const
		{
			return m_shader_;
		}

	private:
		void* m_shader_ = nullptr;
	};

#if CFG_RAYTRACING
	struct CORE_API AccelStructBuffer
	{
		Unique<GraphicMemoryPool> instanceDescPool;
		Unique<GraphicMemoryPool> resultPool;
		Unique<GraphicMemoryPool> scratchPool;

		bool empty = true;
	};
#endif

	struct CORE_API PrimitiveMesh
	{
		virtual      ~PrimitiveMesh() = default;
		virtual void Generate(const Resources::Mesh* mesh) = 0;

	protected:
		virtual void SetNativeVertexBuffer(void* buffer)
		{
			m_vertex_buffer_ = buffer;
		}

		virtual void SetNativeIndexBuffer(void* buffer)
		{
			m_index_buffer_ = buffer;
		}

#if CFG_RAYTRACING
		static AccelStructBuffer& GetAccelStructBuffer(const Resources::Mesh* mesh)
		{
			return mesh->m_blas_;
		}
#endif
		
	private:
		void* m_vertex_buffer_ = nullptr;
		void* m_index_buffer_ = nullptr;
	};

	struct CORE_API CommandListBase
	{
		virtual ~CommandListBase() = default;
		virtual void SoftReset() = 0;
		virtual void FlagReady(const std::function<void()>& post_function = {}) = 0;
		virtual void Execute() = 0;
	};

	struct GraphicInterfaceContextPrimitive;
	
	struct CORE_API GraphicHeapBase
	{
		virtual ~GraphicHeapBase() = default;

		virtual void SetShaderResources(
			const Resources::Texture* const* textures,
			const UINT count,
			const UINT offset) const = 0;
		
		virtual void BindGraphic(const GraphicInterfaceContextPrimitive* cmd) const = 0;
		virtual void BindCompute(const GraphicInterfaceContextPrimitive* cmd) const = 0;
	};

	struct CORE_API GraphicInterfaceContextPrimitive
	{
		CommandListBase* commandList;
		GraphicHeapBase* heap;
	};

	struct CORE_API GraphicResourcePrimitive
	{
	public:
		virtual ~GraphicResourcePrimitive() = default;

		template <typename T>
		T* GetResource()
		{
			return static_cast<T*>(m_resource_);
		}

		template <typename T>
		T** GetAddressOf()
		{
			return static_cast<T**>(&m_resource_);
		}

		virtual void SetResource(void* resource)
		{
			m_resource_ = resource;
		}
		
	private:
		void* m_resource_ = nullptr;
	};

	struct CORE_API GraphicInterfaceContextReturnType
	{
		GraphicInterfaceContextReturnType(const Weak<CommandListBase>& cmd, Unique<GraphicHeapBase>&& heap)
		{
			if (const Strong<CommandListBase>& locked = cmd.lock()) 
			{
				commandList = locked;
			}

			this->heap = std::move(heap);
		}

		GraphicInterfaceContextPrimitive GetPointers() const
		{
			return GraphicInterfaceContextPrimitive
			{
				.commandList = this->commandList.get(),
				.heap = this->heap.get()
			};
		}

	private:
		Strong<CommandListBase> commandList;
		Unique<GraphicHeapBase> heap;
	};

	class CORE_API ConstantBufferTypelessBase
	{
	public:
		virtual ~ConstantBufferTypelessBase() = default;

		virtual void                Create(const void* src_data, const size_t stride) = 0;
		virtual void                SetData(const void* src_data, const size_t stride) = 0;
		[[nodiscard]] virtual void* GetData() const = 0;
		virtual void                Bind(const GraphicInterfaceContextPrimitive* context, const size_t slot) = 0;
	};

	template <typename T>
	class ConstantBufferTypeProxy
	{
	public:
		ConstantBufferTypeProxy() = default;

		explicit ConstantBufferTypeProxy(ConstantBufferTypelessBase* base) : m_base_(base)
		{
			static_assert(std::is_standard_layout_v<T>, "Constant buffer type must be a POD type");
		}

		bool operator!() const
		{
			return m_base_ == nullptr;
		}

		void Create(const T* src_data)
		{
			if (!m_base_) return;
			m_base_->Create(src_data, sizeof(T));
		}

		void SetData(const T* src_data)
		{
			if (!m_base_) return;
			m_base_->SetData(src_data, sizeof(T));
		}

		T GetData() const
		{
			if (!m_base_) throw std::runtime_error("Uninitialized constant buffer");
			return *static_cast<const T*>(m_base_->GetData());
		}

		void Bind(const GraphicInterfaceContextPrimitive* context)
		{
			m_base_->Bind(context, which_cb<T>::value);
		}

	private:
		Unique<ConstantBufferTypelessBase> m_base_;
	};

	class CORE_API StructuredBufferTypelessBase
	{
	public:
		virtual ~StructuredBufferTypelessBase() = default;

		virtual void Create(const GraphicInterfaceContextPrimitive* context, const UINT size, const void* initial_data, const size_t stride, const bool uav) = 0;
		virtual void SetData(const GraphicInterfaceContextPrimitive* context, const UINT size, const void* src_data, const size_t stride) = 0;
		virtual void SetDataContainer(const GraphicInterfaceContextPrimitive* context, const UINT size, const void* const* container_ptr, const size_t stride) = 0;
		virtual void GetData(const GraphicInterfaceContextPrimitive* context, const UINT size, void* dst_ptr, const size_t stride) = 0;
		virtual void Clear() = 0;

		virtual void TransitionToSRV(const GraphicInterfaceContextPrimitive* context) = 0;
		virtual void TransitionToUAV(const GraphicInterfaceContextPrimitive* context) = 0;
		virtual void TransitionCommon(const GraphicInterfaceContextPrimitive* context) = 0;

		virtual void CopySRVHeap(const GraphicInterfaceContextPrimitive* context, const UINT slot) const = 0;
		virtual void CopyUAVHeap(const GraphicInterfaceContextPrimitive* context, const UINT slot) const = 0;
	};

	template <typename T>
	class StructuredBufferTypeProxy
	{
	public:
		StructuredBufferTypeProxy() = default;
		
		explicit StructuredBufferTypeProxy(StructuredBufferTypelessBase* base) : m_base_(base) {}

		bool operator!() const
		{
			return m_base_ == nullptr;
		}

		void Create(const GraphicInterfaceContextPrimitive* context, const UINT size, const T* initial_data)
		{
			if (!m_base_) return;

			m_base_->Create(context, size, initial_data, sizeof(T), is_uav_sb<T>::value || is_client_uav_sb<T>::value);
		}

		void SetData(const GraphicInterfaceContextPrimitive* context, const UINT size, const T* src_data)
		{
			if (!m_base_) return;

			m_base_->SetData(context, size, src_data, sizeof(T));
		}

		void SetDataContainer(const GraphicInterfaceContextPrimitive* context, const UINT size, const T* const* container_ptr)
		{
			if (!m_base_) return;

			m_base_->SetDataContainer(context, size, reinterpret_cast<const void* const*>(container_ptr), sizeof(T));
		}

		void GetData(const GraphicInterfaceContextPrimitive* context, const UINT size, T* dst_ptr)
		{
			if (!m_base_) return;

			m_base_->GetData(context, size, dst_ptr, sizeof(T));
		}

		void CopySRVHeap(const GraphicInterfaceContextPrimitive* context) const
		{
			if (!m_base_) return;

			if constexpr (is_sb<T>::value)
			{
				m_base_->CopySRVHeap(context, which_sb<T>::value);
			}
			else if constexpr(is_client_sb<T>::value)
			{
				m_base_->CopySRVHeap(context, which_client_sb<T>::value);
			}
		}

		void CopyUAVHeap(const GraphicInterfaceContextPrimitive* context) const
		{
			if (!m_base_) return;

			if constexpr (is_uav_sb<T>::value)
			{
				m_base_->CopySRVHeap(context, which_sb_uav<T>::value);
			}
			else if constexpr(is_client_uav_sb<T>::value)
			{
				m_base_->CopySRVHeap(context, which_client_sb_uav<T>::value);
			}
		}

		[[nodiscard]] StructuredBufferTypelessBase& GetTypeless()
		{
			return reinterpret_cast<StructuredBufferTypelessBase&>(*this);
		}

	private:
		Unique<StructuredBufferTypelessBase> m_base_;
	};

	struct CORE_API GraphicInterface
	{
		virtual      ~GraphicInterface() = default;
		
		virtual void Initialize() = 0;
		virtual void Shutdown() = 0;
		virtual void WaitForNextFrame() = 0;
		virtual void Present() = 0;
		
		virtual void* GetNativeInterface() = 0;
		virtual void* GetNativePipeline() = 0;

		virtual PrimitiveTexture* GetNewPrimitiveTexture() = 0;
		virtual PrimitiveMesh* GetNewPrimitiveMesh() = 0;
		virtual GraphicPrimitiveShader* GetNewGraphicPrimitiveShader() = 0;
		virtual ComputePrimitiveShader* GetNewComputePrimitiveShader() = 0;

		virtual Matrix GetProjectionMatrix() = 0;
		virtual Matrix GetOrthogonalMatrix() = 0;

		template <typename T>
		StructuredBufferTypeProxy<T> GetStructuredBuffer() 
		{
			return StructuredBufferTypeProxy<T>(GetNativeStructuredBuffer());
		}

		template <typename T>
		ConstantBufferTypeProxy<T> GetConstantBuffer()
		{
			return ConstantBufferTypeProxy<T>(GetNativeConstantBuffer());
		}

		virtual GraphicInterfaceContextReturnType GetNewContext(const int8_t type, bool heap_allocation, const std::wstring_view debug_name) = 0;
		virtual Strong<CommandListBase> GetCommandList(const int8_t type, const std::wstring_view debug_name) = 0;
		virtual Unique<GraphicHeapBase> GetHeap() = 0;

		virtual void SetViewport(const GraphicInterfaceContextPrimitive* context, const Viewport& viewport) = 0;
		virtual void SetDefaultGraphicPipeline(const GraphicInterfaceContextPrimitive* context) = 0;
		virtual void SetDefaultComputePipeline(const GraphicInterfaceContextPrimitive* context) = 0;

		virtual void Draw(const GraphicInterfaceContextPrimitive* context, const Resources::Shape* shape, const UINT instance_count) = 0;
		virtual void Draw(const GraphicInterfaceContextPrimitive* context, const Resources::Mesh* mesh, const UINT instance_count) = 0;
		virtual void Dispatch(const GraphicInterfaceContextPrimitive* context, const Resources::ComputeShader* shader, const Graphics::SBs::LocalParamSB& local_param, const UINT group_count[3]) = 0;

		virtual void BindGraphic(const GraphicInterfaceContextPrimitive* context, const Resources::Shader* shader) = 0;
		virtual void BindCompute(const GraphicInterfaceContextPrimitive* context, const Resources::ComputeShader* shader) = 0;

		virtual void Bind(const GraphicInterfaceContextPrimitive* context, const Resources::Texture* tex, const eBindType bind_type, const UINT slot, const UINT offset) = 0;
		virtual void BindMultiple(const GraphicInterfaceContextPrimitive* context, const Resources::Texture* const* rtvs, const size_t rtv_count, Resources::Texture* dsv) = 0;
		virtual void BindMultiple(const GraphicInterfaceContextPrimitive* context, const Resources::Texture* const* textures, const eBindType bind_type, const UINT slot, const UINT offset, const size_t count) = 0;
		virtual void Unbind(const GraphicInterfaceContextPrimitive* context, const Resources::Texture* tex, const eBindType bind_type) = 0;
		virtual void UnbindMultiple(const GraphicInterfaceContextPrimitive* context, const Resources::Texture* const* rtvs, const size_t rtv_count, Resources::Texture* dsv) = 0;
		virtual void UnbindMultiple(const GraphicInterfaceContextPrimitive* context, const Resources::Texture* const* textures, const eBindType bind_type, const size_t count) = 0;
		virtual void Clear(const GraphicInterfaceContextPrimitive* context, const Resources::Texture* tex, const eBindType clear_type) = 0;
		virtual void ClearRenderTarget() = 0;

		virtual void CopyRenderTarget(const GraphicInterfaceContextPrimitive* context, const Resources::Texture* tex) = 0;

	protected:
		virtual StructuredBufferTypelessBase* GetNativeStructuredBuffer() = 0;
		virtual ConstantBufferTypelessBase* GetNativeConstantBuffer() = 0;
	};

	struct CORE_API GraphicInterfaceAccessor
	{
	public:
		template <typename T> requires (std::is_base_of_v<GraphicInterface, T>)
		static void SetGraphicInterface()
		{
			if (!s_graphic_interface)
			{
				s_graphic_interface = std::make_unique<T>();
				s_graphic_interface->Initialize();
			}
		}

		[[nodiscard]] static GraphicInterface& GetInterface()
		{
			return *s_graphic_interface;
		}

	private:
		static Unique<GraphicInterface> s_graphic_interface;
	};

	template <typename T>
	class StructuredBufferMemoryPool
	{
	public:
		StructuredBufferMemoryPool() = default;

		StructuredBufferMemoryPool (const StructuredBufferMemoryPool&) = delete;
		StructuredBufferMemoryPool& operator=(const StructuredBufferMemoryPool&) = delete;

		void    resize(const size_t size)
		{
			Update(nullptr, size);
		}

		StructuredBufferTypeProxy<T>& get()
		{
			return m_resource_[m_read_offset_];
		}

		void advance() 
		{
			++m_read_offset_;

			if (m_read_offset_ >= m_allocated_size_)
			{
				resize(m_allocated_size_ * 1.5f);
			}
		}

		void reset() 
		{
			m_used_size_ = 0;
			m_read_offset_ = 0;
		}

		void Update(const T* src_data, size_t count)
		{
			if (count == 0)
			{
				count = 1;
			}

			UpdateSizeIfNeeded(count);

			if (!src_data)
			{
				return;
			}

			Copy(src_data, 0, count);
			m_used_size_ = count;
		}

	private:
		void UpdateSizeIfNeeded(const size_t count)
		{
			if (m_allocated_size_ < count)
			{
				const auto& delta  = count - m_resource_.size();
				size_t      end_it = m_resource_.size();
				m_resource_.resize(count);

				GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();
				const GraphicInterfaceContextReturnType& context = gi.GetNewContext(0, false, L"Structured Buffer Memory pool resizing");
				const GraphicInterfaceContextPrimitive& primitive = context.GetPointers();

				primitive.commandList->SoftReset();

				for (; end_it < count; ++end_it)
				{
					m_resource_.at(end_it) = gi.GetStructuredBuffer<T>();
					m_resource_[end_it].SetData(&primitive, 1, nullptr);
				}

				primitive.commandList->FlagReady();

				m_allocated_size_ = count;
			}
		}

		void Copy(const T* src_data, const size_t offset, const size_t count)
		{
			if (m_resource_.size() < count)
			{
				throw std::logic_error("Memory pool is not allocated enough size");
			}

			GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();
			const GraphicInterfaceContextReturnType& context = gi.GetNewContext(0, false, L"Structured Buffer Memory pool copy");
			const GraphicInterfaceContextPrimitive& primitive = context.GetPointers();

			primitive.commandList->SoftReset();
			
			for (size_t i = offset; i < count; ++i)
			{
				m_resource_[i].SetData(&primitive, 1, src_data);
			}

			primitive.commandList->FlagReady();
		}

		std::vector<StructuredBufferTypeProxy<T>> m_resource_{};
		size_t                                        m_allocated_size_{};
		size_t                                        m_used_size_{};
		size_t                                        m_read_offset_{};
	};

	class CORE_API GraphicMemoryPool
	{
	public:
		GraphicMemoryPool()
			: m_allocated_size_(0),
			  m_used_size_(0) { }

		virtual ~GraphicMemoryPool() { }

		void Update(const void* src_data, size_t count, const size_t stride)
		{
			if (count == 0)
			{
				count = 1;
			}

			if (m_allocated_size_ < count)
			{
				InitializeBuffer(count, stride);
				m_allocated_size_ = count;
			}

			if (!src_data)
			{
				return;
			}

			Map(src_data, count, stride);
				
			m_used_size_ = count;
		}

		virtual void Map(const void* src_data, const size_t count, const size_t stride) = 0;

		void Release()
		{
			m_resource_.reset();
		}

		template <typename T>
		[[nodiscard]] T** GetAddressOf()
		{
			return m_resource_->GetAddressOf<T>();
		}

		template <typename T>
		[[nodiscard]] T* GetResource() const
		{
			return m_resource_->GetResource<T>();
		}

		[[nodiscard]] GraphicResourcePrimitive& GetPrimitive() const
		{
			return *m_resource_;
		}

	private:
		virtual void InitializeBuffer(const size_t count, const size_t stride) = 0;

		Unique<GraphicResourcePrimitive> m_resource_;
		size_t                           m_allocated_size_;
		size_t                           m_used_size_;
	};
}