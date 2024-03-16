#include "pch.h"
#include "egEntity.hpp"

#include "egImGuiHeler.hpp"
#include "imgui_internal.h"
#include "imgui_stdlib.h"

SERIALIZE_IMPL
(
	Engine::Abstract::Entity, 
	_ARTAG(m_name_) 
	_ARTAG(m_meta_str_)
  _ARTAG(m_b_garbage_)
  _ARTAG(m_b_initialized_)
)

void Engine::Abstract::Entity::SetName(const EntityName& name) { m_name_ = name; }

void Engine::Abstract::Entity::SetGarbage(bool garbage) { m_b_garbage_ = garbage; }

const std::filesystem::path& Engine::Abstract::Entity::GetMetadataPath() const { return m_meta_path_; }

Engine::GlobalEntityID Engine::Abstract::Entity::GetID() const { return reinterpret_cast<GlobalEntityID>(this); }

Engine::EntityName Engine::Abstract::Entity::GetName() const { return m_name_; }

Engine::TypeName Engine::Abstract::Entity::GetTypeName() const { return typeid(*this).name(); }

Engine::TypeName Engine::Abstract::Entity::GetPrettyTypeName() const
{
  const auto type_name = GetTypeName();
  const auto pos = type_name.find_last_of(":");

  return type_name.substr(pos + 1);
}

bool Engine::Abstract::Entity::IsGarbage() const { return m_b_garbage_; }

bool Engine::Abstract::Entity::IsInitialized() const { return m_b_initialized_; }

bool& Engine::Abstract::Entity::IsImGuiOpened() { return m_b_imgui_opened_; }

void Engine::Abstract::Entity::Initialize() { m_b_initialized_ = true; }

void Engine::Abstract::Entity::OnSerialized()
{
  m_meta_path_ = m_meta_str_;
}

void Engine::Abstract::Entity::OnDeserialized()
{
  m_meta_path_ = m_meta_str_;
}

void Engine::Abstract::Entity::OnImGui()
{
  lldDisabled("Entity ID", GetID());
  TextAligned("Name", m_name_);
  TextDisabled("Metadata Path", m_meta_str_);

  if (ImGui::Button("Save")) 
  {
	  Serializer::Serialize(GetName(), GetSharedPtr<Entity>());
  }
}

