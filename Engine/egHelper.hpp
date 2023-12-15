#pragma once
#include <imgui.h>
#include "egCommon.hpp"

#undef max

namespace Engine
{
    // instantiate a object, scene, layer.
    template <typename T, typename... Arg>
    inline static boost::shared_ptr<T> Instantiate(Arg&&... args)
    {
        if constexpr (std::is_base_of_v<Abstract::Object, T> ||
                      std::is_base_of_v<Scene, T> || std::is_base_of_v<Layer, T>)
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

    inline static bool __vectorcall IsSamePolarity(const float v1, const float v2)
    {
        return std::copysign(1.0f, v1) == std::copysign(1.0f, v2);
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

    template <typename T>
    inline static std::vector<T> flatten(const std::vector<std::vector<T>>& orig)
    {
        std::vector<T> ret;
        for (const auto& v : orig) ret.insert(ret.end(), v.begin(), v.end());
        return ret;
    }

    inline static void ImGuiVector3Editable(
        const EntityID     id,
        const std::string& var_name,
        Vector3&           v)
    {
        const auto unique_id = std::to_string(id) + var_name;
        ImGui::PushID(unique_id.c_str());
        ImGui::InputFloat("x", &v.x);
        ImGui::InputFloat("y", &v.y);
        ImGui::InputFloat("z", &v.z);
        ImGui::PopID();
    }

    inline static void ImGuiQuaternionEditable(
        const EntityID     id,
        const std::string& var_name,
        Quaternion&        v)
    {
        const auto unique_id = std::to_string(id) + var_name;
        ImGui::PushID(unique_id.c_str());
        ImGui::InputFloat("x", &v.x);
        ImGui::InputFloat("y", &v.y);
        ImGui::InputFloat("z", &v.z);
        ImGui::InputFloat("w", &v.w);
        ImGui::PopID();
    }

    inline static void ImGuiVector2Editable(
        const EntityID     id,
        const std::string& var_name,
        Vector2&           v)
    {
        const auto unique_id = std::to_string(id) + var_name;
        ImGui::PushID(unique_id.c_str());
        ImGui::InputFloat("x", &v.x);
        ImGui::InputFloat("y", &v.y);
        ImGui::PopID();
    }

    inline static void __vectorcall Vector3CheckNanException(const Vector3& v)
    {
        if (std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.z))
        {
            throw std::runtime_error("Vector3CheckNan: NaN detected");
        }
    }
} // namespace Engine
