#pragma once
#include <execution>
#include <functional>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <assimp/matrix4x4.h>
#include <oneapi/tbb.h>

#include "egCommon.hpp"
#include "egMesh.h"
#include "egResourceManager.hpp"

#undef max

namespace Engine
{
	template <typename T, typename ResLock = std::enable_if_t<std::is_base_of_v<Abstract::Resource, T>>>
	inline static boost::weak_ptr<T> Get(const std::string& name)
	{
		return GetResourceManager().GetResource<T>(name);
	}

	inline static void ZeroToEpsilon(Vector3& v)
	{
		if (v.x == 0.0f)
		{
			v.x = g_epsilon;
		}
		if (v.y == 0.0f)
		{
			v.y = g_epsilon;
		}
		if (v.z == 0.0f)
		{
			v.z = g_epsilon;
		}
	}

	template <typename T>
	struct pretty_name
	{
		static std::string get()
		{
			const std::string type_name = typeid(T).name();
			const auto        pos       = type_name.find_last_of(":");
			return type_name.substr(pos + 1);
		}
	};

	inline static bool __vectorcall IsSamePolarity(const float v1, const float v2)
	{
		return std::copysign(1.0f, v1) == std::copysign(1.0f, v2);
	}

	template <typename T>
	inline static std::vector<T> flatten(const std::vector<std::vector<T>>& orig)
	{
		std::vector<T> ret;
		for (const auto& v : orig)
		{
			ret.insert(ret.end(), v.begin(), v.end());
		}
		return ret;
	}

	inline static void __vectorcall Vector3CheckNanException(const Vector3& v)
	{
		if (std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.z))
		{
			throw std::runtime_error
					("Vector3CheckNan: NaN detected");
		}
	}

	inline static Vector3 __vectorcall Vector3Overwrite(const Vector3& vec1, const Vector3& vec2, const Vector3& select)
	{
		return Vector3
		{
			FloatCompare(select.x, 0.f) ? vec1.x : vec2.x,
			FloatCompare(select.y, 0.f) ? vec1.y : vec2.y,
			FloatCompare(select.z, 0.f) ? vec1.z : vec2.z
		};
	}

	inline static Vector3 __vectorcall Vector3SignCopy(const Vector3& vec, const Vector3& sign)
	{
		Vector3 clamped;
		sign.Clamp(-Vector3::One, Vector3::One, clamped);

		return Vector3
		{
			vec.x * clamped.x,
			vec.y * clamped.y,
			vec.z * clamped.z
		};
	}

	inline static Matrix __vectorcall AiMatrixToDirectXTranspose(const aiMatrix4x4& from)
	{
		return Matrix
				(
				 from.a1, from.b1, from.c1, from.d1,
				 from.a2, from.b2, from.c2, from.d2,
				 from.a3, from.b3, from.c3, from.d3,
				 from.a4, from.b4, from.c4, from.d4
				);
	}
} // namespace Engine
