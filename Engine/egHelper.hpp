#pragma once
#include <imgui.h>

#include <assimp/matrix4x4.h>

#include "egCommon.hpp"
#include "egMesh.h"
#include "egResourceManager.hpp"

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

    template <typename T, typename ResLock = std::enable_if_t<std::is_base_of_v<Abstract::Resource, T>>>
    inline static boost::weak_ptr<T> Get(const std::string& name)
    {
        return GetResourceManager().GetResource<T>(name);
    }

    inline static bool __vectorcall IsSamePolarity(const float v1, const float v2)
    {
        return std::copysign(1.0f, v1) == std::copysign(1.0f, v2);
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

    inline static Matrix __vectorcall AiMatrixToDirectX(const aiMatrix4x4& from)
    {
        return Matrix(
                    from.a1, from.a2, from.a3, from.a4,
                    from.b1, from.b2, from.b3, from.b4,
                    from.c1, from.c2, from.c3, from.c4,
                    from.d1, from.d2, from.d3, from.d4);
    }
} // namespace Engine
