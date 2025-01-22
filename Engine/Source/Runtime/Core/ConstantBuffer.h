#pragma once
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "Source/Runtime/Core/GraphicInterface.h"

// Static constant buffer type, this should be added to every constant buffer
#define CB_T(enum_val) static constexpr eCBType cbtype = enum_val;
// Static raytracing constant buffer type, this should be added to every constant buffer
#define RT_CB_T(enum_val) static constexpr eRaytracingCBType cbtype = enum_val;

namespace Engine
{
	enum eCBType : uint8_t;
	enum eRaytracingCBType :uint8_t;

	class ConstantBufferTypelessBase
	{
	public:
		virtual ~ConstantBufferTypelessBase() = default;
		virtual void Bind(const GraphicInterfaceContextReturnType* context) = 0;
	};

	template <typename T>
	class ConstantBufferTypeBase : public ConstantBufferTypelessBase
	{
	public:
		virtual void Create(const T* src_data) = 0;
		virtual void SetData(const T* src_data) = 0;
		virtual T GetData() const = 0;
	};

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