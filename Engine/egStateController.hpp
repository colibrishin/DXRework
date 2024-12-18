#pragma once
#include "egCommon.hpp"

namespace Engine::Components
{
	class StateController : public Abstract::Component
	{
	public:
		COMPONENT_T(COM_T_STATE)

		explicit StateController(const WeakObjectBase& owner);

		template <typename Enum>
		Enum GetState() const
		{
			return static_cast<Enum>(m_state_);
		}

		template <typename Enum>
		Enum GetPreviousState() const
		{
			return static_cast<Enum>(m_previous_state_);
		}

		bool HasStateChanged() const;

		void Initialize() override;
		void PreUpdate(const float& dt) override;

		void OnDeserialized() override;
		void OnImGui() override;

	protected:
		template <typename Enum>
		void SetState(Enum state)
		{
			m_state_ = static_cast<Enum>(state);
		}

	private:
		SERIALIZE_DECL

		int m_state_;
		int m_previous_state_;
	};
} // namespace Engine::Abstract

BOOST_CLASS_EXPORT_KEY(Engine::Components::StateController)
