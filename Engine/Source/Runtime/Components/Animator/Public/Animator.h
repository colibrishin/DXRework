#pragma once
#include "ModuleManager/Public/IModule.h"

#include "Source/Runtime/Core/Component/Public/Component.h"
#include "Source/Runtime/Resources/BaseAnimation/Public/BaseAnimation.h"

POLYMORPHIC_TYPE_MAP(Engine::Components::Animator, Engine::Abstracts::Component)

namespace Engine
{
	struct AnimatorModule;
}

POLYMORPHIC_TYPE_MAP(Engine::AnimatorModule, Engine::IModule)

namespace Engine
{
	struct AnimatorModule : public IModule
	{
		INLINE_COMPILE_TIME_TYPENAME(AnimatorModule)
		void             Initialize() override;
		void             Shutdown() override;
		bool             DynamicLoadable() override;
	};
}

namespace Engine::Components
{
	class ENGINE_ANIMATOR_API Animator final : public Engine::Abstracts::Component
	{
	public:
		INLINE_COMPILE_TIME_TYPENAME(Animator)

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
		SERIALIZE_DECL
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

		UINT  m_animation_id_;
		float m_current_frame_;
		float m_total_dt_;
	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Components::Animator)