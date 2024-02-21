#include "pch.h"
#include "egResource.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>

#include <imgui_stdlib.h>

#include "egImGuiHeler.hpp"

SERIALIZER_ACCESS_IMPL
(
 Engine::Abstract::Resource,
 _ARTAG(_BSTSUPER(Renderable)) 
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
    Renderable::OnImGui();
    ImGui::Indent(2);
    ImGui::Checkbox("Loaded", &m_bLoaded_);
    ImGui::Text("Path : %s", m_path_str_.c_str());
    ImGui::Unindent(2);
    lldDisabled("Local ID", GetLocalID());

    if (ImGui::Button("Save"))
    {
      // todo: future + promise (async)
      serializeImpl();
    }
  }

  Resource::Resource(std::filesystem::path path, eResourceType type)
    : m_bLoaded_(false),
      m_type_(type),
      m_local_id_(g_invalid_id),
      m_path_(std::move(path)) { m_path_str_ = m_path_.string(); }

  Resource::Resource()
    : m_bLoaded_(false),
      m_type_(RES_T_UNK),
      m_local_id_(g_invalid_id) {}

  void Resource::OnDeserialized()
  {
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

  LocalResourceID Resource::GetLocalID() const { return m_local_id_; }

  eResourceType Resource::GetResourceType() const { return m_type_; }
} // namespace Engine::Abstract
