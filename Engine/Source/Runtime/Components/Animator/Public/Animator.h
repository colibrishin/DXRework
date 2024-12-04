#pragma once
#include "Source/Runtime/Abstracts/CoreComponent/Public/Component.h"

namespace Engine::Components
{
	class Animator final : public Engine::Abstracts::Component
	{
	public:
		COMPONENT_T(COM_T_ANIMATOR)

		Animator(const Weak<Engine::Abstracts::ObjectBase>& owner);
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void FixedUpdate(const float& dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		void SetAnimation(UINT idx);

		UINT  GetAnimation() const;
		float GetFrame() const;
		float GetDt() const;

	private:
		SERIALIZE_DECL
		COMP_CLONE_DECL
		Animator();

		void UpdateTransform(const Strong<Transform>& tr, const Strong<Resources::BaseAnimation>& anim) const;

		template <typename T>
		void ResetIfTimer(const boost::shared_ptr<T>& anim)
		{
			if (anim->ConvertDtToFrame(m_total_dt_, anim->GetTicksPerSecond()) >= anim->GetDuration())
			{
				m_current_frame_ = 0.0f;
				m_total_dt_      = 0.0f;
			}
		}

		template <typename T>
		void UpdateTimer(const boost::shared_ptr<T>& anim)
		{
			m_current_frame_ = anim->ConvertDtToFrame(m_total_dt_, anim->GetTicksPerSecond());
		}

		UINT  m_animation_id_;
		float m_current_frame_;
		float m_total_dt_;
	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Components::Animator)
