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

    enum eResourcePriority
    {
        RESOURCE_PRIORITY_SHADER = 0,
        RESOURCE_PRIORITY_TEXTURE,
        RESOURCE_PRIORITY_MESH,
        RESOURCE_PRIORITY_FONT,
        RESOURCE_PRIORITY_SOUND,
    };

    enum eComponentPriority
    {
        COMPONENT_PRIORITY_DEFAULT = 0,
        COMPONENT_PRIORITY_TRANSFORM,
        COMPONENT_PRIORITY_COLLIDER,
        COMPONENT_PRIORITY_RIGIDBODY,
        COMPONENT_PRIORITY_STATE
    };

    enum eBoundingType
    {
        BOUNDING_TYPE_BOX = 0,
        BOUNDING_TYPE_FRUSTUM,
        BOUNDING_TYPE_SPHERE,
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

    union BoundingGroup
    {
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
