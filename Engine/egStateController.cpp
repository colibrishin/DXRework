#include "pch.h"
#include "egStateController.hpp"

#include "egImGuiHeler.hpp"

SERIALIZER_ACCESS_IMPL
(
 Engine::Components::StateController,
 _ARTAG(_BSTSUPER(Engine::Abstract::Component))
 _ARTAG(m_state_)
 _ARTAG(m_previous_state_)
)

namespace Engine::Components
{
  StateController::StateController(const WeakObject& owner)
    : Component(COM_T_STATE, owner),
      m_state_(0),
      m_previous_state_(0) {}

  bool StateController::HasStateChanged() const { return m_state_ != m_previous_state_; }

  void StateController::Initialize()
  {
    Component::Initialize();
    m_state_          = 0;
    m_previous_state_ = 0;
  }

  void StateController::PreUpdate(const float& dt) { m_previous_state_ = m_state_; }

  void StateController::OnDeserialized() { Component::OnDeserialized(); }

  void StateController::OnImGui()
  {
    Component::OnImGui();
    intDisabled("Current State", m_state_);
    intDisabled("Previous State", m_previous_state_);
  }
}
