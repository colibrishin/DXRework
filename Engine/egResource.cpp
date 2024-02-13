#include "pch.h"
#include "egResource.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>

#include <imgui_stdlib.h>

SERIALIZER_ACCESS_IMPL
(
 Engine::Abstract::Resource,
 _ARTAG(_BSTSUPER(Renderable)) _ARTAG(m_bLoaded_)
 _ARTAG(m_path_str_) _ARTAG(m_type_)
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

    if (ImGui::Button("Save"))
    {
      // todo: future + promise (async)
      Serializer::Serialize(GetName(), this);
    }
  }

  Resource::Resource(std::filesystem::path path, eResourceType type)
    : m_bLoaded_(false),
      m_path_(std::move(path)),
      m_type_(type) { m_path_str_ = m_path_.string(); }

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

  eResourceType Resource::GetResourceType() const { return m_type_; }
} // namespace Engine::Abstract
