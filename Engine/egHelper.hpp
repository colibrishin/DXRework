#pragma once
#include "egCommon.hpp"

#undef max

namespace Engine
{
	// instantiate a object, scene, layer.
	template <typename T, typename... Arg>
	inline static boost::shared_ptr<T> Instantiate(Arg&&... args)
	{
		if constexpr (
			std::is_base_of_v<Engine::Abstract::Object, T> ||
			std::is_base_of_v<Engine::Scene, T> ||
			std::is_base_of_v<Engine::Layer, T>)
		{
				const auto inst = boost::make_shared<T>(std::forward<Arg>(args)...);
				inst->Initialize();
				return inst;
		}
		else
		{
			throw std::runtime_error("Instantiate: Invalid type" + typeid(T).name());
		}
	}

	inline static bool IsAssigned(const LONG_PTR id)
	{
		return id != g_invalid_id;
	}

	static bool IsSamePolarity(const float v1, const float v2)
	{
		return std::copysign(1.0f, v1) == std::copysign(1.0f, v2);
	}

	static float MaxElement(const Vector3& v)
	{
		return std::max(std::max(v.x, v.y), v.z);
	}

	static Vector3 MaxUnitVector(const Vector3& v)
	{
		const auto x = std::fabs(v.x);
		const auto y = std::fabs(v.y);
		const auto z = std::fabs(v.z);

		if (x > y && x > z)
			return { std::copysign(1.0f, v.x), 0.0f, 0.0f };
		else if (y > x && y > z)
			return { 0.0f, std::copysign(1.0f, v.y), 0.0f };
		else
			return { 0.0f, 0.0f, std::copysign(1.0f, v.z) };
	}

	static Vector3 RemoveVectorElement(const Vector3& v, const Vector3& condition)
	{
		return {
			std::fabsf(condition.x) == 1.0f ? 0.f : v.x,
			std::fabsf(condition.y) == 1.0f ? 0.f : v.y,
			std::fabsf(condition.z) == 1.0f ? 0.f : v.z };
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
