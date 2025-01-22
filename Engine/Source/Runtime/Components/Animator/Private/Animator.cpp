#include "../Public/Animator.h"
#include "Source/Runtime/Core/Components/Transform/Public/Transform.h"
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.hpp"

#include "Source/Runtime/Components/ModelRenderer/Public/ModelRenderer.h"

#include "Source/Runtime/Resources/AtlasAnimation/Public/AtlasAnimation.h"
#include "Source/Runtime/Resources/BaseAnimation/Public/BaseAnimation.h"
#include "Source/Runtime/Resources/BoneAnimation/Public/BoneAnimation.h"
#include "Source/Runtime/Resources/Material/Public/Material.h"

namespace Engine::Components
{
	COMP_CLONE_IMPL(Animator)

	Animator::Animator(const Weak<Engine::Abstracts::ObjectBase>& owner)
		: Component(COM_T_ANIMATOR, owner),
		  m_animation_id_(0),
		  m_current_frame_(0),
		  m_total_dt_(0) {}

	void Animator::PreUpdate(const float& dt) {}

	void Animator::Update(const float& dt)
	{
		if (!GetActive())
		{
			return;
		}

		const auto mr = GetOwner().lock()->GetComponent<ModelRenderer>();

		if (mr.expired())
		{
			return;
		}
		const auto mat = mr.lock()->GetMaterial();
		if (mat.expired())
		{
			return;
		}

		const auto tr_anim    = mat.lock()->GetResource<Resources::BaseAnimation>(m_animation_id_).lock();
		const auto bone_anim  = mat.lock()->GetResource<Resources::BoneAnimation>(m_animation_id_).lock();
		const auto atlas_anim = mat.lock()->GetResource<Resources::AtlasAnimation>(m_animation_id_).lock();

		m_total_dt_ += dt;

		if (tr_anim)
		{
			ResetIfTimer(tr_anim);
			UpdateTimer(tr_anim);

			if (const auto tr = GetOwner().lock()->GetComponent<Transform>().lock())
			{
				UpdateTransform(tr, tr_anim);
			}
		}
		else if (bone_anim)
		{
			ResetIfTimer(bone_anim);
			UpdateTimer(bone_anim);
		}
		else if (atlas_anim)
		{
			ResetIfTimer(atlas_anim);
			UpdateTimer(atlas_anim);
		}
	}

	void Animator::FixedUpdate(const float& dt) {}

	void Animator::OnSerialized()
	{
		Component::OnSerialized();
	}

	void Animator::OnDeserialized()
	{
		Component::OnDeserialized();
	}

	void Animator::SetAnimation(UINT idx)
	{
		m_animation_id_ = idx;
	}

	UINT Animator::GetAnimation() const
	{
		return m_animation_id_;
	}

	float Animator::GetFrame() const
	{
		return m_current_frame_;
	}

	float Animator::GetDt() const
	{
		return m_total_dt_;
	}

	Animator::Animator()
		: Component(COM_T_ANIMATOR, {}),
		  m_animation_id_(0),
		  m_current_frame_(0),
		  m_total_dt_(0) {}

	void Animator::UpdateTransform(const Strong<Transform>& tr, const Strong<Resources::BaseAnimation>& anim) const
	{
		if (anim)
		{
			const auto time = Resources::BaseAnimation::ConvertDtToFrame
					(
					 m_current_frame_,
					 anim->GetTicksPerSecond()
					);

			const auto& primitive = anim->m_simple_primitive_;

			const auto pos   = primitive.GetPosition(time);
			const auto rot   = primitive.GetRotation(time);
			const auto scale = primitive.GetScale(time);

			tr->SetAnimationPosition(pos);
			tr->SetAnimationRotation(rot);
			tr->SetAnimationScale(scale);
		}
		else
		{
			tr->SetAnimationPosition(Vector3::Zero);
			tr->SetAnimationRotation(Quaternion::Identity);
			tr->SetAnimationScale(Vector3::One);
		}
	}
}
