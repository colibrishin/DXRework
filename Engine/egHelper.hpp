#pragma once
#include <imgui.h>
#include "egCommon.hpp"
#include "egMesh.h"

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

    inline static VertexElement& AccessShapeSerialized(std::vector<Resources::Shape>& shapes, const UINT index)
    {
        UINT total_vertices = 0;

        for (int i = 0; i < shapes.size(); ++i)
        {
            total_vertices += static_cast<UINT>(shapes[i].size());
            if (index < total_vertices)
            {
                const auto remainder = (total_vertices - shapes[i].size());
                return shapes[i][index - remainder];
            }
        }

        throw std::runtime_error("AccessShapeSerialized: Invalid index");
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
