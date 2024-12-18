#pragma once

#include "Source/Runtime/Core/Resource/Public/Resource.h"
#include "Source/Runtime/Managers/RenderPipeline/Public/RenderTask.h"
#include "Source/Runtime/Managers/RenderPipeline/Public/RenderType.h"

// Static texture type, this should be added to every texture.
#define TEX_T(enum_val) static constexpr eTexType textype = enum_val;

namespace Engine
{
	struct PrimitiveTexture;

	enum TEXTURE_API eResourceFlag
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

	enum TEXTURE_API eTextureLayout : uint8_t
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

	enum TEXTURE_API eTexType
	{
		TEX_TYPE_UNKNOWN = -1,
		TEX_TYPE_1D,
		TEX_TYPE_2D,
		TEX_TYPE_3D,
		TEX_TYPE_BUFFER,
	};

	enum TEXTURE_API eUAVNativeType
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

	enum TEXTURE_API eUAVBufferFlag
	{
		BUFFER_UAV_FLAG_NONE = 0,
		BUFFER_UAV_FLAG_RAW = 0x1
	};

	struct TEXTURE_API BufferUAVDescription
	{
		UINT64 FirstElement;
		UINT NumElements;
		UINT StructureByteStride;
		UINT64 CounterOffsetInBytes;
		eUAVBufferFlag Flags;
	};

	struct TEXTURE_API Tex1dUAVDescription
	{
		UINT MipSlice;
	};

	struct TEXTURE_API Tex1dArrayUAVDescription
	{
		UINT MipSlice;
		UINT FirstArraySlice;
		UINT ArraySize;
	};

	struct TEXTURE_API Tex2dUAVDescription
	{
		UINT MipSlice;
		UINT PlaneSlice;
	};

	struct TEXTURE_API Tex2dArrayUAVDescription
	{
		UINT MipSlice;
		UINT FirstArraySlice;
		UINT ArraySize;
		UINT PlaneSlice;
	};

	struct TEXTURE_API Tex2dMsUAVDescription
	{
		UINT UnusedField_NothingToDefine;
	};

	struct TEXTURE_API Tex2dMsArrayUAVDescription
	{
		UINT FirstArraySlice;
		UINT ArraySize;
	};

	struct TEXTURE_API Tex3dUAVDescription
	{
		UINT MipSlice;
		UINT FirstWSlice;
		UINT WSize;
	};

	struct TEXTURE_API UAVDescription
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

	struct TEXTURE_API BufferRtvDescription
	{
		UINT64 FirstElement;
		UINT NumElements;
	};

	struct TEXTURE_API Tex1dRtvDescription
	{
		UINT MipSlice;
	};

	struct TEXTURE_API Tex1dArrayRtvDescription
	{
		UINT MipSlice;
		UINT FirstArraySlice;
		UINT ArraySize;
	};

	struct TEXTURE_API Tex2dRtvDescription
	{
		UINT MipSlice;
		UINT PlaneSlice;
	};

	struct TEXTURE_API Tex2dMsRtvDescription
	{
		UINT UnusedField_NothingToDefine;
	};

	struct TEXTURE_API Tex2dArrayRtvDescription
	{
		UINT MipSlice;
		UINT FirstArraySlice;
		UINT ArraySize;
		UINT PlaneSlice;
	};

	struct TEXTURE_API Tex2dMsArrayRtvDescription
	{
		UINT FirstArraySlice;
		UINT ArraySize;
	};

	struct TEXTURE_API Tex3dRtvDescription
	{
		UINT MipSlice;
		UINT FirstWSlice;
		UINT WSize;
	};

	enum TEXTURE_API eNativeRtvType
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

	struct TEXTURE_API RtvDescription
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

	struct TEXTURE_API Tex1dDsvDescription
	{
		UINT MipSlice;
	};

	struct TEXTURE_API Tex1dArrayDsvDescription
	{
		UINT MipSlice;
		UINT FirstArraySlice;
		UINT ArraySize;
	};

	struct TEXTURE_API Tex2dDsvDescription
	{
		UINT MipSlice;
	};

	struct TEXTURE_API Tex2dArrayDsvDescription
	{
		UINT MipSlice;
		UINT FirstArraySlice;
		UINT ArraySize;
	};

	struct TEXTURE_API Tex2dMsDsvDescription
	{
		UINT UnusedField_NothingToDefine;
	};

	struct TEXTURE_API Tex2dMsArrayDsvDescription
	{
		UINT FirstArraySlice;
		UINT ArraySize;
	};


	enum TEXTURE_API eDsvFlag
	{
		DSV_FLAG_NONE = 0,
		DSV_FLAG_READ_ONLY_DEPTH = 0x1,
		DSV_FLAG_READ_ONLY_STENCIL = 0x2
	};

	enum TEXTURE_API eNativeDsvType
	{
		DSV_DIMENSION_UNKNOWN = 0,
		DSV_DIMENSION_TEXTURE1D = 1,
		DSV_DIMENSION_TEXTURE1DARRAY = 2,
		DSV_DIMENSION_TEXTURE2D = 3,
		DSV_DIMENSION_TEXTURE2DARRAY = 4,
		DSV_DIMENSION_TEXTURE2DMS = 5,
		DSV_DIMENSION_TEXTURE2DMSARRAY = 6
	};

	struct TEXTURE_API DsvDescription
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

	enum TEXTURE_API eSrvFlag
	{
		BUFFER_SRV_FLAG_NONE = 0,
		BUFFER_SRV_FLAG_RAW = 0x1
	};

	struct TEXTURE_API BufferSrvDescription
	{
		UINT64 FirstElement;
		UINT NumElements;
		UINT StructureByteStride;
		eSrvFlag Flags;
	};

	struct TEXTURE_API Tex1dSrvDescription
	{
		UINT MostDetailedMip;
		UINT MipLevels;
		FLOAT ResourceMinLODClamp;
	};

	struct TEXTURE_API Tex1dArraySrvDescription
	{
		UINT MostDetailedMip;
		UINT MipLevels;
		UINT FirstArraySlice;
		UINT ArraySize;
		FLOAT ResourceMinLODClamp;
	};

	struct TEXTURE_API Tex2dSrvDescription
	{
		UINT MostDetailedMip;
		UINT MipLevels;
		UINT PlaneSlice;
		FLOAT ResourceMinLODClamp;
	};

	struct TEXTURE_API Tex2dArraySrvDescription
	{
		UINT MostDetailedMip;
		UINT MipLevels;
		UINT FirstArraySlice;
		UINT ArraySize;
		UINT PlaneSlice;
		FLOAT ResourceMinLODClamp;
	};

	struct TEXTURE_API Tex3dSrvDescription
	{
		UINT MostDetailedMip;
		UINT MipLevels;
		FLOAT ResourceMinLODClamp;
	};

	struct TEXTURE_API TexCubeSrvDescription
	{
		UINT MostDetailedMip;
		UINT MipLevels;
		FLOAT ResourceMinLODClamp;
	};

	struct TEXTURE_API TexCubeArraySrvDescription
	{
		UINT MostDetailedMip;
		UINT MipLevels;
		UINT First2DArrayFace;
		UINT NumCubes;
		FLOAT ResourceMinLODClamp;
	};

	struct TEXTURE_API Tex2dMsSrvDescription
	{
		UINT UnusedField_NothingToDefine;
	};

	struct TEXTURE_API Tex2dMsArraySrvDescription
	{
		UINT FirstArraySlice;
		UINT ArraySize;
	};

	struct TEXTURE_API AccelStructSrvDescription
	{
		uint64_t Location; //todo: address type;
	};

	enum TEXTURE_API eNativeSrvType
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

	struct TEXTURE_API SrvDescription
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

	struct TEXTURE_API GenericTextureDescription
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
}

namespace Engine::Resources
{
	class TEXTURE_API Texture : public Abstracts::Resource
	{
	public:
		RESOURCE_T(RES_T_TEX)
		explicit Texture(std::filesystem::path path, eTexType type, const GenericTextureDescription& description);
		~Texture() override = default;

	public:
		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PostUpdate(const float& dt) override;
		void FixedUpdate(const float& dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		[[nodiscard]] eResourceType GetResourceType() const override;
		[[nodiscard]] eTexType GetPrimitiveTextureType() const;
		[[nodiscard]] const GenericTextureDescription& GetDescription() const;
		[[nodiscard]] PrimitiveTexture* GetPrimitiveTexture() const;

		bool IsHotload() const;
		RESOURCE_SELF_INFER_GETTER_DECL(Texture)

	protected:
		// Derived class should hide these by their own case.
		virtual UINT64 GetWidth() const;
		virtual UINT   GetHeight() const;
		virtual UINT   GetDepth() const;

		virtual void Map() {};
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		friend struct Engine::PrimitiveTexture;
		
		Texture();
		void UpdateDescription(const GenericTextureDescription& description);

		GenericTextureDescription m_desc_;
		std::unique_ptr<PrimitiveTexture> m_primitive_texture_;
		eTexType m_type_;
	};
} // namespace Engine::Resources


namespace Engine 
{
	struct TextureMappingTask;
	struct TextureBindingTask;

	struct TEXTURE_API PrimitiveTexture
	{
		virtual ~PrimitiveTexture() = default;
		virtual void Generate(const Weak<Resources::Texture>& texture) = 0;
		virtual void LoadFromFile(const Weak<Resources::Texture>& texture, const std::filesystem::path& path) = 0;
		virtual void SaveAsFile(const std::filesystem::path& path) = 0;

		void UpdateDescription(const Weak<Resources::Texture>& texture, const GenericTextureDescription& description);
		[[nodiscard]] void* GetPrimitiveTexture() const;
		[[nodiscard]] TextureMappingTask& GetMappingTask() const;
		[[nodiscard]] TextureBindingTask& GetBindingTask() const;
		[[nodiscard]] const GenericTextureDescription& GetDescription() const;

	protected:
		void SetPrimitiveTexture(void* texture);
		
		template <typename T> requires (std::is_base_of_v<TextureMappingTask, T>)
		void SetTextureMappingTask()
		{
			if (s_mapping_task_ == nullptr)
			{
				s_mapping_task_ = std::make_unique<T>();
			}
		}

		template <typename T> requires (std::is_base_of_v<TextureBindingTask, T>) 
		void SetTextureBindingTask() 
		{
			if (s_binding_task_ == nullptr) 
			{
				s_binding_task_ = std::make_unique<T>();
			}
		}

	private:
		static std::unique_ptr<TextureMappingTask> s_mapping_task_;
		static std::unique_ptr<TextureBindingTask> s_binding_task_;
		GenericTextureDescription m_description_;
		void* m_texture_ = nullptr;
	};

	struct TEXTURE_API TextureBindingTask 
	{
		virtual ~TextureBindingTask() = default;
		virtual void Bind(RenderPassTask* task_context, PrimitiveTexture* texture, const eBindType bind_type, const UINT bind_slot, const UINT offset) = 0;
		virtual void Unbind(RenderPassTask* task_context, PrimitiveTexture* texture, const eBindType previous_bind_type) = 0;

		virtual void BindMultiple(RenderPassTask* task_context, PrimitiveTexture* const* rtvs, const size_t rtv_count, PrimitiveTexture* dsv) = 0;
		virtual void UnbindMultiple(RenderPassTask* task_context, PrimitiveTexture* const* rtvs, const size_t rtv_count, PrimitiveTexture* dsv) = 0;
	};

	struct TEXTURE_API TextureMappingTask
	{
		virtual ~TextureMappingTask() = default;
		virtual void Map(
			PrimitiveTexture* texture,
			void* data_ptr,
			const size_t width,
			const size_t height,
			const size_t stride,
			const size_t depth = 1) = 0;
		virtual void Map(
			PrimitiveTexture* lhs,
			PrimitiveTexture* rhs,
			const UINT src_width,
			const UINT src_height,
			const size_t src_idx,
			const UINT dst_x,
			const UINT dst_y,
			const size_t dst_idx) = 0;
	};

	struct TEXTURE_API TextureDsvClearTask
	{
		virtual ~TextureDsvClearTask() = default;
		virtual void Run(
			RenderPassTask* render_pass,
			PrimitiveTexture* texture,
			float clear_depth,
			int clear_stencil) = 0;

		virtual void Cleanup() = 0;
	};
}