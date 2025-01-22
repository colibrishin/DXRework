#pragma once
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

// Static constant buffer type, this should be added to every constant buffer
#define CB_T(enum_val) static constexpr eCBType cbtype = enum_val;
// Static raytracing constant buffer type, this should be added to every constant buffer
#define RT_CB_T(enum_val) static constexpr eRaytracingCBType cbtype = enum_val;

namespace Engine
{
	enum eCBType : uint8_t;
	enum eRaytracingCBType :uint8_t;

	namespace Graphics::CBs 
	{
		struct PerspectiveCB
		{
			CB_T(CB_TYPE_WVP)

			Matrix world;
			Matrix view;
			Matrix projection;

			Matrix invView;
			Matrix invProj;
			Matrix invVP;

			Matrix reflectView;
		};

		struct ParamCB : public ParamBase
		{
			CB_T(CB_TYPE_PARAM)
		};

		static_assert(sizeof(ParamCB) % sizeof(Vector4) == 0);

		struct ViewportCB
		{
			RT_CB_T(RAYTRACING_CB_VIEWPORT)
			Vector2 resolution;
		};
	}

	template <typename T>
	struct which_cb
	{
		static constexpr eCBType value = T::cbtype;
	};
}