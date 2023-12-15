#pragma once
#include "egCommon.hpp"
#include "egIStateController.hpp"

namespace Engine::Abstract
{
    template <typename StateEnum,
              class EC = std::enable_if_t<std::is_enum_v<StateEnum>>>
    class StateController : public IStateController
    {
    public:
        StateEnum GetState() const
        {
            return m_state_;
        }

        StateEnum GetPreviousState() const
        {
            return m_previous_state_;
        }

        bool HasStateChanged() const
        {
            return m_state_ != m_previous_state_;
        }

        void Initialize() override;
        void PreUpdate(const float& dt) override;

        void     OnDeserialized() override;
        void     OnImGui() override;
        TypeName GetVirtualTypeName() const final;

    protected:
        StateController()
        : IStateController(COMPONENT_PRIORITY_STATE, {}) {}

        StateController(const WeakObject& owner)
        : IStateController(COMPONENT_PRIORITY_STATE, owner) {}

        void SetState(StateEnum state)
        {
            m_state_ = state;
        }

    private:
        friend class Serializer;
        friend class boost::serialization::access;

        template <class Archive>
        void serialize(Archive& ar, const unsigned int file_version)
        {
            ar & boost::serialization::base_object<Component>(*this);
            ar & m_state_;
            ar & m_previous_state_;
        }

        StateEnum m_state_;
        StateEnum m_previous_state_;
    };

    template <typename StateEnum, class EC>
    void StateController<StateEnum, EC>::Initialize()
    {
        Component::Initialize();
        m_state_          = static_cast<StateEnum>(0);
        m_previous_state_ = static_cast<StateEnum>(0);
    }

    template <typename StateEnum, class EC>
    void StateController<StateEnum, EC>::PreUpdate(const float& dt)
    {
        m_previous_state_ = m_state_;
    }

    template <typename StateEnum, class EC>
    void StateController<StateEnum, EC>::OnDeserialized()
    {
        Component::OnDeserialized();
    }

    template <typename StateEnum, class EC>
    void StateController<StateEnum, EC>::OnImGui()
    {
        Component::OnImGui();
        ImGui::Text("State: %d", m_state_);
        ImGui::Text("Previous State: %d", m_previous_state_);
    }

    template <typename StateEnum, class EC>
    TypeName StateController<StateEnum, EC>::GetVirtualTypeName() const
    {
        return typeid(StateController<StateEnum, EC>).name();
    }
} // namespace Engine::Abstract
