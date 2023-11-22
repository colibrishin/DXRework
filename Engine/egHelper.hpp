#pragma once
#include "egCommon.hpp"

#undef max

namespace Engine
{
	template <typename T, typename... Arg>
	inline static std::shared_ptr<T> Instantiate(Arg&&... args)
	{
		const auto object = std::make_shared<T>(std::forward<Arg>(args)...);
		object->Initialize();
		return object;
	}

	static bool IsSamePolarity(const float v1, const float v2)
	{
		return std::copysign(1.0f, v1) == std::copysign(1.0f, v2);
	}

	static float MaxElement(const Vector3& v)
	{
		return std::max(std::max(v.x, v.y), v.z);
	}

	static bool FloatCompare(const float a, const float b)
	{
		return std::fabs(a - b) < g_epsilon * std::fmaxf(1.0f, std::fmaxf(std::fabsf(a), std::fabsf(b)));
	}

	static Vector3 VectorElementAdd(const Vector3& lhs, const float value)
	{
		return { lhs.x + value, lhs.y + value, lhs.z + value };
	}

	static bool VectorElementInRange(const Vector3& lhs, const float value)
	{
		return std::max(std::max(lhs.x, lhs.y), lhs.z) < value;
	}

	static Vector3 XMTensorCross(const DirectX::XMFLOAT3X3& lhs, const Vector3& rhs)
	{
		return {
			lhs._11 * rhs.x + lhs._12 * rhs.y + lhs._13 * rhs.z,
			lhs._21 * rhs.x + lhs._22 * rhs.y + lhs._23 * rhs.z,
			lhs._31 * rhs.x + lhs._32 * rhs.y + lhs._33 * rhs.z
		};
	}

	template <typename T>
	std::vector<T> flatten(const std::vector<std::vector<T>>& orig)
	{
		std::vector<T> ret;
		for (const auto& v : orig)
			ret.insert(ret.end(), v.begin(), v.end());
		return ret;
	}  
}
