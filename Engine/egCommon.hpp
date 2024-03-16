#pragma once
#include <boost/smart_ptr/weak_ptr.hpp>

#include "egDXCommon.h"
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
      if (!lhs.lock()) { return true; }
      if (!rhs.lock()) { return false; }

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

  inline static bool IsAssigned(const LONG_PTR id) { return id != g_invalid_id; }

  inline static float __vectorcall MaxElement(const Vector3& v) { return std::max(std::max(v.x, v.y), v.z); }

  inline static Vector3 __vectorcall MaxUnitVector(const Vector3& v)
  {
    const auto x = std::fabs(v.x);
    const auto y = std::fabs(v.y);
    const auto z = std::fabs(v.z);

    if (x > y && x > z) { return {std::copysign(1.0f, v.x), 0.0f, 0.0f}; }
    if (y > x && y > z) { return {0.0f, std::copysign(1.0f, v.y), 0.0f}; }
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

  inline static bool __vectorcall FloatCompare(const float a, const float b)
  {
    return std::fabs(a - b) <
           g_epsilon * std::fmaxf(1.0f, std::fmaxf(std::fabsf(a), std::fabsf(b)));
  }

  inline static Vector3 __vectorcall VectorElementAdd(const Vector3& lhs, const float value)
  {
    return {lhs.x + value, lhs.y + value, lhs.z + value};
  }

  inline static bool __vectorcall VectorElementInRange(const Vector3& lhs, const float value)
  {
    return std::max(std::max(lhs.x, lhs.y), lhs.z) < value;
  }

  inline static Vector3 __vectorcall XMTensorCross(const XMFLOAT3X3& lhs, const Vector3& rhs)
  {
    return {
      lhs._11 * rhs.x + lhs._12 * rhs.y + lhs._13 * rhs.z,
      lhs._21 * rhs.x + lhs._22 * rhs.y + lhs._23 * rhs.z,
      lhs._31 * rhs.x + lhs._32 * rhs.y + lhs._33 * rhs.z
    };
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
