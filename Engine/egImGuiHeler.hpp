#pragma once
#include <imgui.h>
#include <imgui_stdlib.h>

namespace Engine
{
  inline static std::string _labelPrefix(const char* const label)
  {
    const float width = ImGui::CalcItemWidth();
    const float x     = ImGui::GetCursorPosX();

    ImGui::Text(label);
    ImGui::SameLine();
    ImGui::SetCursorPosX(x + width * 0.75f + ImGui::GetStyle().ItemInnerSpacing.x);
    ImGui::SetNextItemWidth(-1);

    std::string labelID = "##";
    labelID += label;

    return labelID;
  }

  inline static void TextDisabled(const std::string& label, const std::string& text)
  {
    ImGui::InputText
      (_labelPrefix(label.c_str()).c_str(), const_cast<std::string*>(&text), ImGuiInputTextFlags_ReadOnly);
  }

  inline static void FloatDisabled(const std::string& label, const float& value)
  {
    ImGui::InputFloat
      (
       _labelPrefix(label.c_str()).c_str(), const_cast<float*>(&value), 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly
      );
  }

  inline static void lldDisabled(const std::string& label, const long long& value)
  {
    ImGui::InputScalar
      (
       _labelPrefix(label.c_str()).c_str(), ImGuiDataType_S64, const_cast<long long*>(&value),
       nullptr, nullptr, nullptr, ImGuiInputTextFlags_ReadOnly
      );
  }

  inline static void intDisabled(const std::string& label, const int& value)
  {
    ImGui::InputScalar
      (
       _labelPrefix(label.c_str()).c_str(), ImGuiDataType_S32, const_cast<int*>(&value),
       nullptr, nullptr, nullptr, ImGuiInputTextFlags_ReadOnly
      );
  }

  inline static void FloatAligned(const std::string& label, float& value)
  {
    ImGui::InputFloat(_labelPrefix(label.c_str()).c_str(), &value);
  }

  inline static void TextAligned(const std::string& label, std::string& text)
  {
    ImGui::InputText(_labelPrefix(label.c_str()).c_str(), &text);
  }

  inline static void CheckboxAligned(const std::string& label, bool& flag)
  {
    ImGui::Checkbox(_labelPrefix(label.c_str()).c_str(), &flag);
  }

  inline static void UINTAligned(const std::string& label, UINT& value)
  {
    ImGui::InputScalar
      (
       _labelPrefix(label.c_str()).c_str(), ImGuiDataType_U32, &value,
       nullptr, nullptr, nullptr, ImGuiInputTextFlags_CharsDecimal
      );
  }

  inline static void lldAligned(const std::string& label, long long& value)
  {
    ImGui::InputScalar
      (
       _labelPrefix(label.c_str()).c_str(), ImGuiDataType_S64, &value,
       nullptr, nullptr, nullptr, ImGuiInputTextFlags_CharsDecimal
      );
  }

  inline static void ImGuiColorEditable(
    const std::string&   label,
    const GlobalEntityID id,
    const std::string&   var_name,
    Color&               c
  )
  {
    const auto  unique_id = std::to_string(id) + var_name;
    const float width     = ImGui::CalcItemWidth();
    const float x         = ImGui::GetCursorPosX();

    ImGui::PushID(unique_id.c_str());
    ImGui::Text(label.c_str());
    ImGui::SameLine();
    ImGui::SetCursorPosX(x + width * 0.75f + ImGui::GetStyle().ItemInnerSpacing.x);

    ImGui::ColorEdit4("##color", &c.x);
    ImGui::PopID();
  }

  inline static bool ImGuiVector3Editable(
    const std::string&   label,
    const GlobalEntityID id,
    const std::string&   var_name,
    Vector3&             v,
    const float          drag_step = 0.1f,
    const float          min_value  = 0,
    const float          max_value  = 0
  )
  {
    const auto  unique_id = std::to_string(id) + var_name;
    const float width     = ImGui::CalcItemWidth();
    const float x         = ImGui::GetCursorPosX();

    ImGui::PushID(unique_id.c_str());
    ImGui::Text(label.c_str());
    ImGui::SameLine();
    ImGui::SetCursorPosX(x + width * 0.75f + ImGui::GetStyle().ItemInnerSpacing.x);

    bool changed = false;

    changed = ImGui::DragFloat3("##vector", &v.x, drag_step, min_value, max_value);
    ImGui::PopID();

    return changed;
  }

  inline static void ImGuiVector2Editable(
    const std::string&   label,
    const GlobalEntityID id,
    const std::string&   var_name,
    Vector2&             v
  )
  {
    const auto  unique_id = std::to_string(id) + var_name;
    const float width     = ImGui::CalcItemWidth();
    const float x         = ImGui::GetCursorPosX();

    ImGui::PushID(unique_id.c_str());
    ImGui::Text(label.c_str());
    ImGui::SameLine();
    ImGui::SetCursorPosX(x + width * 0.75f + ImGui::GetStyle().ItemInnerSpacing.x);

    ImGui::DragFloat2("##vector", &v.x);
    ImGui::PopID();
  }
}
