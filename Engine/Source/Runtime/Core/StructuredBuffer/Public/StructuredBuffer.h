#pragma once
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

// Static structured buffer type, this should be added to every structured buffer
#define SB_T(enum_val) static constexpr eSBType sbtype = enum_val;
#define CLIENT_SB_T(enum_val) static constexpr eClientSBType csbtype = enum_val;

// Static structured buffer UAV type, this should be added to every structured buffer UAV
#define CLIENT_SB_UAV_T(enum_val) static constexpr eClientSBUAVType csbuavtype = enum_val;
#define SB_UAV_T(enum_val) static constexpr eSBUAVType sbuavtype = enum_val;

namespace Engine 
{
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
		struct ENGINE_CORE_API LocalParamSB : public ParamBase
		{
			SB_T(SB_TYPE_LOCAL_PARAM)
		};

		struct ENGINE_CORE_API InstanceSB : public ParamBase
		{
			SB_T(SB_TYPE_INSTANCE)
			SB_UAV_T(SB_TYPE_UAV_INSTANCE)

			void SetTextureSlot(const UINT offset, const UINT slot_id)
			{
				assert(offset < g_max_texture_per_material);
				assert(slot_id < BIND_SLOT_TEXARR);
				SetParam<int>(11 + offset, slot_id);
			}
		};
	}
}