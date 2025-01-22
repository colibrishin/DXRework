#pragma once
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "Source/Runtime/Core/GraphicInterface.h"

// Static structured buffer type, this should be added to every structured buffer
#define SB_T(enum_val) static constexpr eSBType sbtype = enum_val;
#define CLIENT_SB_T(enum_val) static constexpr eClientSBType csbtype = enum_val;

// Static structured buffer UAV type, this should be added to every structured buffer UAV
#define CLIENT_SB_UAV_T(enum_val) static constexpr eClientSBUAVType csbuavtype = enum_val;
#define SB_UAV_T(enum_val) static constexpr eSBUAVType sbuavtype = enum_val;

namespace Engine 
{
	class StructuredBufferTypelessBase
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

		virtual void CopySRVHeap(const GraphicInterfaceContextPrimitive* context) const = 0;
		virtual void CopyUAVHeap(const GraphicInterfaceContextPrimitive* context) const = 0;
	};

	template <typename T>
	class StructuredBufferTypeInterface 
	{
		virtual void Create(const GraphicInterfaceContextPrimitive* context, const UINT size, const T* initial_data, const bool uav) = 0;
		virtual void SetData(const GraphicInterfaceContextPrimitive* context, const UINT size, const T* src_data) = 0;
		virtual void SetDataContainer(const GraphicInterfaceContextPrimitive* context, const UINT size, const T* const* container_ptr) = 0;
		virtual void GetData(const GraphicInterfaceContextPrimitive* context, const UINT size, T* dst_ptr) = 0;
		virtual void Clear() = 0;
	};

	class StructuedBufferMemoryPoolTypelessBase 
	{
	public:
		virtual ~StructuedBufferMemoryPoolTypelessBase() = default;
	};

	template <typename T>
	class StructuredBufferMemoryPoolBase : public StructuedBufferMemoryPoolTypelessBase
	{
		virtual void resize(const size_t size) = 0;

		virtual StructuredBufferTypeBase<T>& get() = 0;

		virtual void advance() 
		{
			++m_read_offset_;
		}

		virtual void reset() 
		{
			m_used_size_ = 0;
			m_read_offset_ = 0;
		}

		virtual void Update(const T* src_data, size_t count) = 0;

	private:
		size_t                                m_allocated_size_;
		size_t                                m_used_size_;
		size_t                                m_read_offset_;
	};

	template <typename T>
	struct which_sb
	{
		static constexpr eSBType value = T::sbtype;
	};

	template <typename T>
	struct which_client_sb
	{
		static constexpr eClientSBType value = T::csbtype;
	};

	template <typename T>
	struct which_sb_uav
	{
		static constexpr eSBUAVType value = T::sbuavtype;
	};

	template <typename T>
	struct which_client_sb_uav
	{
		static constexpr eClientSBUAVType value = T::csbuavtype;
	};

	template <typename T, typename = void>
	struct is_uav_sb : std::false_type {};

	template <typename T>
	struct is_uav_sb<T, std::void_t<decltype(T::csbuavtype == true)>> : std::true_type {};

	template <typename T, typename = void>
	struct is_client_uav_sb : std::false_type {};

	template <typename T>
	struct is_client_uav_sb<T, std::void_t<decltype(T::csbuavtype == true)>> : std::true_type {};

	template <typename T, typename = void>
	struct is_sb : std::false_type {};

	template <typename T>
	struct is_sb<T, std::void_t<decltype(T::sbtype == true)>> : std::true_type {};

	template <typename T, typename = void>
	struct is_client_sb : std::false_type {};

	template <typename T>
	struct is_client_sb<T, std::void_t<decltype(T::csbtype == true)>> : std::true_type {};

	namespace Graphics::SBs 
	{
		struct LocalParamSB : public ParamBase
		{
			SB_T(SB_TYPE_LOCAL_PARAM)
		};

		struct InstanceSB : public ParamBase
		{
			SB_T(SB_TYPE_INSTANCE)
			SB_UAV_T(SB_TYPE_UAV_INSTANCE)
		};
	}
}
