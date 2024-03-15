#include "pch.h"
#include "egPrefab.h"

#include "egSceneManager.hpp"

SERIALIZE_IMPL
(
  Engine::Resources::Prefab,
  _ARTAG(_BSTSUPER(Engine::Abstract::Resource))
  _ARTAG(m_object_)
  _ARTAG(m_children_)
)

namespace Engine::Resources
{
  Prefab::Prefab(const StrongObjectBase& object)
    : Resource("", RES_T_PREFAB)
  {
    // Clone object and remove from scene for isolation.
    const auto& cloned = object->Clone(false);
    cloned->GetScene().lock()->RemoveGameObject(cloned->GetID(), object->GetLayer());
    cloned->SetName(object->GetName());

    for (const auto& child : cloned->GetChildren())
    {
      if (const auto& locked = child.lock())
      {
        m_children_.push_back(Create(locked->GetName(), "", locked));
      }
    }
  }

  void Prefab::PreUpdate(const float& dt) {}

  void Prefab::Update(const float& dt) {}

  void Prefab::PostUpdate(const float& dt) {}

  void Prefab::FixedUpdate(const float& dt) {}

  void Prefab::PreRender(const float& dt) {}

  void Prefab::Render(const float& dt) {}

  void Prefab::PostRender(const float& dt) {}

  void Prefab::OnDeserialized()
  {
    Resource::OnDeserialized();
  }

  void Prefab::OnSerialized()
  {
    Resource::OnSerialized();
  }

  void Prefab::OnImGui()
  {
    Resource::OnImGui();

    static int selected = 0;

    ImGui::Combo("Layer", &selected, g_layer_type_str, std::size(g_layer_type_str));

    if (ImGui::Button("Extract"))
    {
      if (const auto& scene = GetSceneManager().GetActiveScene().lock())
      {
        ExtractObject(scene, (eLayerType)selected);
      }
    }
  }

  void Prefab::ExtractObject(const StrongScene& scene, const eLayerType layer) const
  {
    // Clone from prefab object.
    const auto& cloned = m_object_->Clone(false);
    // Note that removing from scene flips the garbage flag, need to be reset.
    // todo: re-evaluate the necessity of garbage flag.
    cloned->SetGarbage(false);

    // Add to scene. This need to be done first before reconstructing children
    // due to the local id assignment.
    scene->AddGameObject(layer, cloned);

    // Reconstruct children.
    for (const auto& child : m_children_)
    {
      const auto& cloned_child = child->m_object_->Clone(false);
      cloned_child->SetGarbage(false);
      cloned->AddChild(cloned_child);
    }
  }

  void Prefab::Load_INTERNAL() {}

  void Prefab::Unload_INTERNAL() {}

  Prefab::Prefab()
    : Resource("", RES_T_PREFAB) { }
}
