#pragma once
#include <DirectXMath.h>
#include <directxtk12/SimpleMath.h>

namespace Engine
{
	struct MathExtension
	{
		using Vector3 = DirectX::SimpleMath::Vector3;

		inline static bool check_avx()
		{
			// todo: less platform specific way
			constexpr size_t minimum_avx_requirements = 6; // == std::_Stl_isa_available_avx2
			static bool use_avx = std::__isa_available >= minimum_avx_requirements;
			return use_avx;
		}

		inline static float __vectorcall MaxElement(const Vector3& v)
		{
			return std::max(std::max(v.x, v.y), v.z);
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
			if (check_avx())
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
			if (check_avx())
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
