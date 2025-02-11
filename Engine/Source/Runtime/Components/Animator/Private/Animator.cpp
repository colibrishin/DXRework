#include "../Public/Animator.h"
#include "Animator.generated.h"

#include "ModuleManager/Public/ModuleManager.h"

#include "Source/Runtime/Core/Components/Transform/Public/Transform.h"
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.h"

#include "Source/Runtime/Components/ModelRenderer/Public/ModelRenderer.h"

#include "Source/Runtime/Resources/AtlasAnimation/Public/AtlasAnimation.h"
#include "Source/Runtime/Resources/BaseAnimation/Public/BaseAnimation.h"
#include "Source/Runtime/Resources/BoneAnimation/Public/BoneAnimation.h"
#include "Source/Runtime/Resources/Material/Public/Material.h"

namespace Engine::Components
{
	Animator::Animator(const Weak<Engine::Abstracts::ObjectBase>& owner)
		: Component(owner),
		  m_animation_id_(0),
		  m_current_frame_(0),
		  m_total_dt_(0) {}

	void Animator::PreUpdate(const float dt) {}

	void Animator::Update(const float dt)
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

		const auto shape = mr.lock()->GetShape();

		if (const auto& locked = shape.lock())
		{
			m_total_dt_ += dt;
			int duration = 0;
			
			// todo: multiple application of animations
			if (const Strong<Resources::BaseAnimation>& tr_anim = locked->GetTransformAnimation().lock())
			{
				duration = tr_anim->GetDuration();
				ResetIfTimer(tr_anim);
				UpdateTimer(tr_anim);

				if (const auto tr = GetOwner().lock()->GetComponent<Transform>().lock())
				{
					UpdateTransform(tr, tr_anim);
				}
			}
			else if (const Strong<Resources::AnimationTexture> anim_tex = locked->GetAnimations().lock())
			{
				if (const std::vector<Weak<Resources::BoneAnimation>>& bone_anims = anim_tex->GetAnimations();
					bone_anims.size() > m_animation_id_ && !bone_anims[m_animation_id_].expired())
				{
					const Strong<Resources::BoneAnimation>& bone_anim = bone_anims[m_animation_id_].lock();
					duration = bone_anim->GetDuration();
					ResetIfTimer(bone_anim);
					UpdateTimer(bone_anim);
				}
			}
			else if (const Strong<Resources::Material>& mat = locked->GetMaterial(0).lock(); mat && !mat->GetAtlasTexture().expired())
			{
				const Strong<Resources::AtlasAnimation>& atlas_anim = mat->GetAtlasAnimation(m_animation_id_).lock();
				duration = atlas_anim->GetDuration();
				ResetIfTimer(atlas_anim);
				UpdateTimer(atlas_anim);

				AtlasFramePrimitive current_frame;
				atlas_anim->GetFrame(m_current_frame_, current_frame);

				m_primitive_.atlasX = current_frame.X;
				m_primitive_.atlasY = current_frame.Y;
				m_primitive_.atlasW = current_frame.Width;
				m_primitive_.atlasH = current_frame.Height;
			}

			m_primitive_.animationID = m_animation_id_;
			m_primitive_.animationDuration = duration;
			m_primitive_.currentFrame = m_current_frame_;
		}
	}

	void Animator::FixedUpdate(const float dt) {}

	void Animator::OnSerialized()
	{
		Component::OnSerialized();
	}

	void Animator::OnDeserialized()
	{
		Component::OnDeserialized();
	}

	eComponentUpdatePriorities Animator::GetUpdatePriority() const
	{
		return eComponentUpdatePriority::COM_PRIORITY_RENDER;
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

	const Graphics::AnimatorPrimitive& Animator::GetPrimitive() const
	{
		return m_primitive_;
	}

	Animator::Animator()
		: Component({}),
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
