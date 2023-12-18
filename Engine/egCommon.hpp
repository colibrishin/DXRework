#pragma once
#include <wrl/client.h>

#include <DirectXCollision.h>
#include <SimpleMath.h>

#include <boost/make_shared.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/weak_ptr.hpp>

#include "egDXCommon.h"
#include "egSerialization.hpp"
#include "egType.h"

#include <fmod.h>

#undef max
#undef min

namespace Engine
{
    constexpr float g_epsilon     = 0.001f;
    constexpr float g_gravity_acc = 9.8f;

    constexpr float  g_fixed_update_interval   = 0.02f;
    constexpr int    g_debug_y_movement        = 15;
    constexpr int    g_debug_y_initial         = 0;
    constexpr float  g_debug_message_life_time = 1.0f;
    constexpr size_t g_debug_message_max       = 20;

    constexpr bool g_speculation_enabled = true;

    constexpr LONG_PTR g_invalid_id = -1;

    constexpr Vector3 g_forward  = {0.f, 0.f, 1.f};
    constexpr Vector3 g_backward = {0.f, 0.f, -1.f};

    constexpr size_t g_max_map_size             = 2048; // only in power of 2
    constexpr size_t g_octree_negative_round_up = g_max_map_size / 2;

    inline std::atomic<UINT> g_collision_energy_reduction_multiplier = 2;

    enum eLayerType
    {
        LAYER_NONE = 0,
        LAYER_LIGHT,
        LAYER_DEFAULT,
        LAYER_ENVIRONMENT,
        LAYER_SKYBOX,
        LAYER_UI,
        LAYER_CAMERA,
        LAYER_MAX
    };

    enum eObserverState
    {
        OBSERVER_STATE_NONE,
    };

    enum eResourceType
    {
        RES_T_UNK = 0,
        RES_T_SHADER,
        RES_T_TEX,
        RES_T_NORMAL,
        RES_T_MESH,
        RES_T_FONT,
        RES_T_SOUND,
    };

    enum eComponentType
    {
        COM_T_UNK = 0,
        COM_T_TRANSFORM,
        COM_T_COLLIDER,
        COM_T_RIDIGBODY,
        COM_T_STATE,
        COM_T_MESH_RENDERER
    };

    enum eDefObjectType
    {
        DEF_OBJ_T_UNK = 0,
        DEF_OBJ_T_NONE,
        DEF_OBJ_T_CAMERA,
        DEF_OBJ_T_LIGHT,
        DEF_OBJ_T_OBSERVER,
        DEF_OBJ_T_TEXT,
        DEF_OBJ_T_DELAY_OBJ
    };

    // THIS ENUM SHOULD BE DEFINED AT THE CLIENT!
    enum eSceneType : UINT;

    enum eBoundingType
    {
        BOUNDING_TYPE_BOX = 0,
        BOUNDING_TYPE_FRUSTUM,
        BOUNDING_TYPE_SPHERE,
    };

    template <typename T>
    struct which_resource
    {
        static constexpr eResourceType value = T::rtype;
    };

    template <typename T>
    struct which_component
    {
        static constexpr eComponentType value = T::ctype;
    };

    template <typename T>
    struct which_def_object
    {
        static constexpr eDefObjectType value = T::dotype;
    };

    template <typename T>
    struct which_scene
    {
        static constexpr eSceneType value = T::stype;
    };

    struct GUIDComparer
    {
        bool operator()(const GUID& Left, const GUID& Right) const
        {
            return memcmp(&Left, &Right, sizeof(Right)) < 0;
        }
    };

    template <typename T>
    struct WeakComparer
    {
        bool operator()(
            const boost::weak_ptr<T>& lhs,
            const boost::weak_ptr<T>& rhs) const
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

    struct ResourcePriorityComparer
    {
        bool operator()(
            const StrongResource& Left,
            const StrongResource& Right) const;
    };

    struct ComponentPriorityComparer
    {
        bool operator()(const WeakComponent& Left, const WeakComponent& Right) const;
    };

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

        if (x > y && x > z) return {std::copysign(1.0f, v.x), 0.0f, 0.0f};
        if (y > x && y > z) return {0.0f, std::copysign(1.0f, v.y), 0.0f};
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

    template <typename BoundingType>
    __forceinline static BoundingType __vectorcall TranslateBounding(
        const BoundingType& box, const Vector3& scale, const Quaternion& rotation, const Vector3& position)
    {
        if constexpr (std::is_same_v<BoundingType, BoundingOrientedBox>)
        {
            auto box_ = const_cast<BoundingOrientedBox&>(box);
            box_.Transform(box_, 1.f, rotation, position);
            box_.Extents = Vector3(box_.Extents.x * scale.x, box_.Extents.y * scale.y, box_.Extents.z * scale.z);
            return box_;
        }
        else if constexpr (std::is_same_v<BoundingType, BoundingSphere>)
        {
            auto sphere_ = const_cast<BoundingSphere&>(box);
            sphere_.Transform(sphere_, MaxElement(scale), rotation, position);
            return sphere_;
        }
        else
        {
            static_assert("TranslateBounding: Invalid type");
            return {};
        }
    }

    union BoundingGroup
    {
    public:
        BoundingGroup() : box(Vector3::Zero, {0.5f, 0.5f, 0.5f}, Quaternion::Identity) {}

        template <typename BoundingType>
        __forceinline BoundingType __vectorcall As(const Vector3& scale, const Quaternion& rotation, const Vector3& translation) const
        {
            if constexpr (std::is_same_v<BoundingType, BoundingOrientedBox>)
            {
                const auto box_ = TranslateBounding(box, scale, rotation, translation);
                return box_;
            }
            else if constexpr (std::is_same_v<BoundingType, BoundingSphere>)
            {
                const auto sphere_ = TranslateBounding(sphere, scale, rotation, translation);
                return sphere_;
            }
            else
            {
                static_assert("TranslateBounding: Invalid type");
                return {};
            }
        }

        template <typename BoundingType>
        void CreateFromPoints(size_t count, const Vector3* points, size_t stride)
        {
            if (std::is_same_v<BoundingType, BoundingOrientedBox>)
            {
                box.CreateFromPoints(box, count, points, stride);
            }
            else if (std::is_same_v<BoundingType, BoundingSphere>)
            {
                sphere.CreateFromPoints(sphere, count, points, stride);
            }
            else if (std::is_same_v<BoundingType, BoundingBox>)
            {
                BoundingBox box_;
                BoundingBox::CreateFromPoints(box_, count, points, stride);
                BoundingOrientedBox::CreateFromBoundingBox(box, box_);
            }
            else
            {
                static_assert("TranslateBounding: Invalid type");
            }
        }

    private:
        BoundingOrientedBox box;
        BoundingSphere      sphere;
    };
} // namespace Engine

namespace DX
{
    // Helper class for COM exceptions
    class com_exception : public std::exception
    {
    public:
        com_exception(HRESULT hr)
        : result(hr) {}

        const char* what() const noexcept override
        {
            static char s_str[64] = {};
            sprintf_s(
                      s_str, "Failure with HRESULT of %08X",
                      static_cast<unsigned int>(result));
            return s_str;
        }

    private:
        HRESULT result;
    };

    // Helper utility converts D3D API failures into exceptions.
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw com_exception(hr);
        }
    }
} // namespace DX

namespace FMOD::DX
{
    // Helper class for COM exceptions
    class fmod_exception : public std::exception
    {
    public:
        fmod_exception(FMOD_RESULT hr)
        : result(hr) {}

        const char* what() const noexcept override
        {
            static char s_str[64] = {};
            sprintf_s(
                      s_str, "Failure with FMOD_RESULT of %08X",
                      static_cast<unsigned int>(result));
            return s_str;
        }

    private:
        HRESULT result;
    };

    // Helper utility converts D3D API failures into exceptions.
    inline void ThrowIfFailed(FMOD_RESULT hr)
    {
        if (hr != FMOD_OK)
        {
            throw fmod_exception(hr);
        }
    }
} // namespace FMOD::DX
