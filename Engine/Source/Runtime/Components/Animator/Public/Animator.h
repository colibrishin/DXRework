#pragma once
#include "ModuleManager/Public/IModule.h"

#include "Source/Runtime/Core/Component/Public/Component.h"
#include "Source/Runtime/Resources/BaseAnimation/Public/BaseAnimation.h"

#include "Animator.generated.h"

namespace Engine::Components
{
	ECLASS(serialize)
	class ENGINE_ANIMATOR_API Animator final : public Engine::Abstracts::Component
	{
		GENERATE_BODY
	public:
		Animator(const Weak<Engine::Abstracts::ObjectBase>& owner);
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void FixedUpdate(const float dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		eComponentUpdatePriority GetUpdatePriority() const override;
		void SetAnimation(UINT idx);

		UINT  GetAnimation() const;
		float GetFrame() const;
		float GetDt() const;

	private:
		COMP_CLONE_DECL
		Animator();

		void UpdateTransform(const Strong<Transform>& tr, const Strong<Resources::BaseAnimation>& anim) const;

		template <typename T>
		void ResetIfTimer(const Strong<T>& anim)
		{
			if (anim->ConvertDtToFrame(m_total_dt_, anim->GetTicksPerSecond()) >= anim->GetDuration())
			{
				m_current_frame_ = 0.0f;
				m_total_dt_      = 0.0f;
			}
		}

		template <typename T>
		void UpdateTimer(const Strong<T>& anim)
		{
			m_current_frame_ = anim->ConvertDtToFrame(m_total_dt_, anim->GetTicksPerSecond());
		}

		EPROPERTY()
		UINT  m_animation_id_;
		
		EPROPERTY()
		float m_current_frame_;
		
		EPROPERTY()
		float m_total_dt_;
	};
}
