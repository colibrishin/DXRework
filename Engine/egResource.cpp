#include "pch.h"
#include "egResource.h"

#include <imgui_stdlib.h>

#include "egImGuiHeler.hpp"

SERIALIZE_IMPL
(
 Engine::Abstract::Resource,
 _ARTAG(_BSTSUPER(Entity)) 
 _ARTAG(m_bLoaded_)
 _ARTAG(m_path_str_)
 _ARTAG(m_type_)
)

namespace Engine::Abstract
{
  Resource::~Resource() {}

  void Resource::Load()
  {
    if (!m_bLoaded_)
    {
      Load_INTERNAL();
      m_bLoaded_ = true;
    }
  }

  void Resource::Unload()
  {
    if (m_bLoaded_)
    {
      Unload_INTERNAL();
      m_bLoaded_ = false;
    }
  }

  void Resource::OnImGui()
  {
    ImGui::BulletText(GetTypeName().c_str());
    Entity::OnImGui();
    ImGui::Indent(2);
    ImGui::Checkbox("Loaded", &m_bLoaded_);
    TextDisabled("Resource Path", m_path_str_);
    ImGui::Unindent(2);
  }

  Resource::Resource(std::filesystem::path path, eResourceType type)
    : m_bLoaded_(false),
      m_type_(type),
      m_path_(std::move(path)) { m_path_str_ = m_path_.string(); }

  Resource::Resource()
    : m_bLoaded_(false),
      m_type_(RES_T_UNK) {}

  void Resource::OnDeserialized()
  {
    Entity::OnDeserialized();
    m_bLoaded_ = false;
    m_path_    = m_path_str_;
  }

  bool Resource::IsLoaded() const { return m_bLoaded_; }

  const std::filesystem::path& Resource::GetPath() const { return m_path_; }

  void Resource::SetPath(const std::filesystem::path& path)
  {
    m_path_     = std::move(path);
    m_path_str_ = m_path_.generic_string();
  }

  eResourceType Resource::GetResourceType() const { return m_type_; }
} // namespace Engine::Abstract
