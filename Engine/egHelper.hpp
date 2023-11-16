#pragma once
#include "egApplication.hpp"
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

	static Vector3 XMTensorCross(const DirectX::XMFLOAT3X3& lhs, const Vector3& rhs)
	{
		return {
			lhs._11 * rhs.x + lhs._12 * rhs.y + lhs._13 * rhs.z,
			lhs._21 * rhs.x + lhs._22 * rhs.y + lhs._23 * rhs.z,
			lhs._31 * rhs.x + lhs._32 * rhs.y + lhs._33 * rhs.z
		};
	}

	static Matrix GenerateWorldMatrix(const DirectX::BoundingOrientedBox& box)
	{
		Matrix world = Matrix::CreateWorld(Vector3::Zero, Vector3::Forward, Vector3::Up);

		const Matrix S = Matrix::CreateScale(box.Extents * 2.f);
		const Matrix R = Matrix::CreateFromQuaternion(box.Orientation);
		const Matrix T = Matrix::CreateTranslation(box.Center);

		world *= S * R * T;
		return world;
	}
}
