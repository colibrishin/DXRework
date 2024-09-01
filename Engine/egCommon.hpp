#pragma once
#include <boost/smart_ptr/weak_ptr.hpp>

#include "egEnums.h"
#include "egSerialization.hpp"
#include "egType.h"

#undef max
#undef min

namespace Engine::Physics
{
	struct GenericBounding;
}

namespace Engine
{
	using DirectX::SimpleMath::Vector3;
	using Microsoft::WRL::ComPtr;

	template <typename T>
	struct WeakComparer
	{
		bool operator()(
			const boost::weak_ptr<T>& lhs,
			const boost::weak_ptr<T>& rhs
		) const
		{
			if (!lhs.lock())
			{
				return true;
			}
			if (!rhs.lock())
			{
				return false;
			}

			return lhs.lock().get() < rhs.lock().get();
		}
	};

	inline std::string Vector3ToString(const Vector3& v)
	{
		return std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z);
	}

	struct ResourcePriorityComparer
	{
		bool operator()(
			StrongResource Left,
			StrongResource Right
		) const;
	};

	struct ComponentPriorityComparer
	{
		bool operator()(WeakComponent Left, WeakComponent Right) const;
	};

	struct bounding_getter
	{
		static Physics::GenericBounding value(Abstract::ObjectBase& object);
	};

	struct CollisionInfo
	{
		WeakObjectBase lhs;
		WeakObjectBase rhs;

		bool speculative;
		bool collision;
	};

	static bool check_avx()
	{
		constexpr size_t minimum_avx_requirements = 6; // == std::_Stl_isa_available_avx2
		static bool use_avx = std::__isa_available >= minimum_avx_requirements;
		return use_avx;
	}


	static void _mm256_memcpy_Impl(void* dst, const void* src, const size_t size)
	{
		// 32 bytes size block copy
		const size_t count = size / sizeof(__m256);
		// remaining bytes if size is not multiple of 32
		const size_t remain = size % sizeof(__m256);

		const auto p_dst = static_cast<__m256i*>(dst);
		const auto p_src = static_cast<const __m256i*>(src);

		for (size_t i = 0; i < count; ++i)
		{
			_mm256_store_si256(p_dst + i, *(p_src + i));
		}

		// If remaining bytes exist, fallback to default memcpy
		if (remain)
		{
			std::memcpy(p_dst + count, p_src + count, remain);
		}
	}

	static void _mm256_memcpy(void* dst, const void* src, const size_t size)
	{
		if (check_avx())
		{
			_mm256_memcpy_Impl(dst, src, size);
		}
		else
		{
			std::memcpy(dst, src, size);
		}
	}

	// todo: Using this function would remove the const qualifier from the object.
	template <typename T>
	inline static bool __vectorcall LockWeak(const boost::weak_ptr<T>& weak, boost::shared_ptr<T>& strong)
	{
		if (weak.expired())
		{
			return false;
		}
		strong = weak.lock();
		return true;
	}

	inline static bool IsAssigned(const LONG_PTR id)
	{
		return id != g_invalid_id;
	}

	inline static float __vectorcall MaxElement(const Vector3& v)
	{
		return std::max(std::max(v.x, v.y), v.z);
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

	static bool __vectorcall FloatCompare(const float a, const float b)
	{
		return std::fabs(a - b) <
		       g_epsilon * std::fmaxf(1.0f, std::fmaxf(std::fabsf(a), std::fabsf(b)));
	}


	static bool __vectorcall Vector3Compare(const Vector3& lhs, const Vector3& rhs)
	{
		return FloatCompare(lhs.x, rhs.x) && FloatCompare(lhs.y, rhs.y) && FloatCompare(lhs.z, rhs.z);
	}

	static Vector3 __vectorcall VectorElementAdd(const Vector3& lhs, const float value)
	{
		if (check_avx())
		{
			const __m128 v = _mm_set_ps(lhs.x, lhs.y, lhs.z, 0.f);
			return _mm_add_ps(v, _mm_set1_ps(value));
		}
		return {lhs.x + value, lhs.y + value, lhs.z + value};
	}

	static bool __vectorcall VectorElementInRange(const Vector3& lhs, const float value)
	{
		return std::max(std::max(lhs.x, lhs.y), lhs.z) < value;
	}

	static Vector3 __vectorcall XMTensorCross(const XMFLOAT3X3& lhs, const Vector3& rhs)
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

	template <size_t... Indices, typename T, typename... Args>
	auto mem_bind_impl(T* this_pointer, void(T::*function)(Args...))
	{
		return std::bind(function, this_pointer, std::_Ph<Indices>{}...);
	}

	template <size_t... Indices, typename T, typename... Args>
	auto mem_bind_impl(T* this_pointer, void(T::*function)(Args...) const)
	{
		return std::bind(function, this_pointer, std::_Ph<Indices>{}...);
	}

	template <typename T, typename... Args>
	auto mem_bind(T* this_pointer, void(T::*function)(Args...))
	{
		return mem_bind_impl<sizeof...(Args)>(this_pointer, function);
	}

	template <typename T, typename... Args>
	auto mem_bind(T* this_pointer, void(T::*function)(Args...) const)
	{
		return mem_bind_impl<sizeof...(Args)>(this_pointer, function);
	}

	template <typename T, typename U>
	void CheckSize(const U compare_value, const std::wstring_view out_string)
	{
		if constexpr (g_debug)
		{
			if (compare_value > std::numeric_limits<T>::max() || compare_value < std::numeric_limits<T>::min())
			{
				OutputDebugStringW(out_string.data());
			}
		}
	}
} // namespace Engine

namespace DX
{
	// Helper class for COM exceptions
	class com_exception : public std::exception
	{
	public:
		com_exception(HRESULT hr)
			: result(hr) {}

		const char* what() const noexcept override;

	private:
		HRESULT result;
	};

	void ThrowIfFailed(HRESULT hr);
} // namespace DX

enum FMOD_RESULT;

namespace FMOD::DX
{
	// Helper class for COM exceptions
	class fmod_exception : public std::exception
	{
	public:
		fmod_exception(FMOD_RESULT hr)
			: result(hr) {}

		const char* what() const noexcept override;

	private:
		FMOD_RESULT result;
	};

	void ThrowIfFailed(FMOD_RESULT hr);
} // namespace FMOD::DX
