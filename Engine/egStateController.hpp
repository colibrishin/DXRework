#pragma once

namespace Engine::Abstract
{
	template <typename StateEnum>
	class StateController : public Component
	{
	public:
		StateEnum GetState() const { return m_state_; }
		StateEnum GetPreviousState() const { return m_previous_state_; }
		bool HasStateChanged() const { return m_state_ != m_previous_state_; }

	protected:
		StateController(const WeakObject& owner) : Component(COMPONENT_PRIORITY_STATE, owner) {}

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void SetState(StateEnum state) { m_state_ = state; }
		void OnLayerChanging() override;
		void OnLayerChanged() override;

	private:
		StateEnum m_state_;
		StateEnum m_previous_state_;

	};

	template <typename StateEnum>
	void StateController<StateEnum>::Initialize()
	{
		m_state_ = (StateEnum)0;
		m_previous_state_ = (StateEnum)0;
	}

	template <typename StateEnum>
	void StateController<StateEnum>::PreUpdate(const float& dt)
	{
		m_previous_state_ = m_state_;
	}

	template <typename StateEnum>
	void StateController<StateEnum>::OnLayerChanging()
	{
	}

	template <typename StateEnum>
	void StateController<StateEnum>::OnLayerChanged()
	{
	}
}
