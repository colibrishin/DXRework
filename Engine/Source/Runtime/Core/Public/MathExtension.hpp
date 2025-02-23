#pragma once
#define _USE_MATH_DEFINES
#include <math.h>

#include "TypeLibrary.h"
#include "SIMDExtension.hpp"

namespace Engine
{
	struct MathExtension
	{
		inline static float ToRadian(const float degree)
		{
			return degree * M_PI / 180.f;
		}

		inline static float ToDegree(const float radian)
		{
			return M_PI * 180.f / M_PI;
		}

		inline static Vector3 __vectorcall ToEuler(const Quaternion& q)
		{
			const auto& getZ = [&]()
			{
				float siny_cosp = 2 * (q.w * q.z + q.x * q.y);
				float cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
				float z = std::atan2f(siny_cosp, cosy_cosp);
				return z;
			};

			const auto& getY = [&]()
			{
				float sinp = std::sqrt(1 + 2 * (q.w * q.y - q.x * q.z));
				float cosp = std::sqrt(1 - 2 * (q.w * q.y - q.x * q.z));
				float y = 2 * std::atan2f(sinp, cosp) - M_PI / 2;
				return y;
			};

			const auto& getX = [&]()
			{
				float sinr_cosp = 2 * (q.w * q.x + q.y * q.z);
				float cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y);
				float x = std::atan2f(sinr_cosp, cosr_cosp);
				return x;
			};

			// Z-Forward, Y-Up coordination system.
			return { getX(), getY(), getZ() };
		}

		inline static Quaternion __vectorcall ToQuaternion(float pitch, float yaw, float roll) 
		{
			// Abbreviations for the various angular functions
			float cr = std::cosf(roll * 0.5f);
			float sr = std::sinf(roll * 0.5f);
			float cp = std::cosf(pitch * 0.5f);
			float sp = std::sinf(pitch * 0.5f);
			float cy = std::cosf(yaw * 0.5f);
			float sy = std::sinf(yaw * 0.5f);

			Quaternion q;
			q.w = cr * cp * cy + sr * sp * sy;
			q.x = sr * cp * cy - cr * sp * sy;
			q.y = cr * sp * cy + sr * cp * sy;
			q.z = cr * cp * sy - sr * sp * cy;

			return q;
		}

		inline static float __vectorcall MaxElement(const Vector3& v)
		{
			return std::max(std::max(v.x, v.y), v.z);
		}

		inline static bool __vectorcall IsSamePolarity(const float v1, const float v2)
		{
			return std::copysign(1.0f, v1) == std::copysign(1.0f, v2);
		}

		inline static void ZeroToEpsilon(Vector3& v, const float epsilon = 0.0001f)
		{
			if (v.x == 0.0f)
			{
				v.x = epsilon;
			}
			if (v.y == 0.0f)
			{
				v.y = epsilon;
			}
			if (v.z == 0.0f)
			{
				v.z = epsilon;
			}
		}

		inline static void __vectorcall Vector3CheckNanException(const Vector3& v)
		{
			if (std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.z))
			{
				throw std::runtime_error
				("Vector3CheckNan: NaN detected");
			}
		}

		inline static Vector3 __vectorcall MaxUnitVector(const Vector3& v)
		{
			const auto x = std::fabs(v.x);
			const auto y = std::fabs(v.y);
			const auto z = std::fabs(v.z);

			if (x > y && x > z)
			{
				return {std::copysign(1.0f, v.x), 0.0f, 0.0f};
			}
			if (y > x && y > z)
			{
				return {0.0f, std::copysign(1.0f, v.y), 0.0f};
			}
			return {0.0f, 0.0f, std::copysign(1.0f, v.z)};
		}

		inline static Vector3 __vectorcall RemoveVectorElement(const Vector3& v, const Vector3& condition)
		{
			return {
				std::fabsf(condition.x) == 1.0f ? 0.f : v.x,
				std::fabsf(condition.y) == 1.0f ? 0.f : v.y,
				std::fabsf(condition.z) == 1.0f ? 0.f : v.z
			};
		}

		inline static bool __vectorcall FloatCompare(const float a, const float b, const float epsilon = 1e-03)
		{
			return std::fabs(a - b) <
			       epsilon * std::fmaxf(1.0f, std::fmaxf(std::fabsf(a), std::fabsf(b)));
		}

		inline static bool __vectorcall Vector3Compare(const Vector3& lhs, const Vector3& rhs)
		{
			return FloatCompare(lhs.x, rhs.x) && FloatCompare(lhs.y, rhs.y) && FloatCompare(lhs.z, rhs.z);
		}

		inline static Vector3 __vectorcall VectorElementAdd(const Vector3& lhs, const float value)
		{
			if (SIMDExtension::check_avx())
			{
				const __m128 v = _mm_set_ps(lhs.x, lhs.y, lhs.z, 0.f);
				return _mm_add_ps(v, _mm_set1_ps(value));
			}

			return {lhs.x + value, lhs.y + value, lhs.z + value};
		}

		inline static bool __vectorcall VectorElementInRange(const Vector3& lhs, const float value)
		{
			return std::max(std::max(lhs.x, lhs.y), lhs.z) < value;
		}

		inline static Vector3 __vectorcall XMTensorCross(const XMFLOAT3X3& lhs, const Vector3& rhs)
		{
			if (SIMDExtension::check_avx())
			{
				const __m128 v   = _mm_set_ps(rhs.x, rhs.y, rhs.z, 0.f);
				__m128       mr0 = _mm_set_ps(lhs._11, lhs._12, lhs._13, 0.f);
				__m128       mr1 = _mm_set_ps(lhs._21, lhs._22, lhs._23, 0.f);
				__m128       mr2 = _mm_set_ps(lhs._31, lhs._32, lhs._33, 0.f);

				mr0 = _mm_mul_ps(v, mr0);
				mr1 = _mm_mul_ps(v, mr1);
				mr2 = _mm_mul_ps(v, mr2);

				const Vector3 result =
				{
					_mm_hadd_ps(mr0, mr0).m128_f32[1],
					_mm_hadd_ps(mr1, mr1).m128_f32[1],
					_mm_hadd_ps(mr2, mr2).m128_f32[1]
				};

				return result;
			}

			return
			{
				lhs._11 * rhs.x + lhs._12 * rhs.y + lhs._13 * rhs.z,
				lhs._21 * rhs.x + lhs._22 * rhs.y + lhs._23 * rhs.z,
				lhs._31 * rhs.x + lhs._32 * rhs.y + lhs._33 * rhs.z
			};
		}
	};
}
