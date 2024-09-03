#include "pch.h"
#include "clPlayerScript.h"

#include "clHitboxScript.hpp"
#include "clPlayerHitboxScript.h"
#include "clRIfleScript.h"
#include "egAnimator.h"
#include "egBaseCollider.hpp"
#include "egBoneAnimation.h"
#include "egCamera.h"
#include "egCollisionDetector.h"
#include "egImGuiHeler.hpp"
#include "egMaterial.h"
#include "egModelRenderer.h"
#include "egMouseManager.h"
#include "egRigidbody.h"
#include "egShape.h"
#include "egText.h"

SERIALIZE_IMPL
(
 Client::Scripts::PlayerScript,
 _ARTAG(_BSTSUPER(Script)) _ARTAG(m_hp_) _ARTAG(m_state_) _ARTAG(m_prev_state_)
 _ARTAG(m_top_view_) _ARTAG(m_cam_id_)
)

namespace Client::Scripts
{
	SCRIPT_CLONE_IMPL(PlayerScript)

	void PlayerScript::Initialize()
	{
		Script::Initialize();
		SetState(CHAR_STATE_IDLE);

		if (const auto& owner = GetOwner().lock())
		{
			const auto& tr   = owner->AddComponent<Components::Transform>().lock();
			const auto& mr   = owner->AddComponent<Components::ModelRenderer>().lock();
			const auto& rb   = owner->AddComponent<Components::Rigidbody>().lock();
			const auto& atr  = owner->AddComponent<Components::Animator>().lock();
			const auto& cldr = owner->AddComponent<Components::Collider>().lock();

			const auto model = Resources::Shape::Get("CharacterShape").lock();
			mr->SetMaterial(Resources::Material::Get("Character"));

			cldr->SetShape(model);
			cldr->SetType(BOUNDING_TYPE_BOX);
			cldr->SetMass(1.0f);

			rb->SetFrictionCoefficient(0.1f);
			rb->SetGravityOverride(true);
			rb->SetNoAngular(true);

			atr->SetAnimation(0);
		}

		// todo: determine local player
		//MoveCameraToChild(true);
	}

	void PlayerScript::PreUpdate(const float& dt)
	{
		m_prev_state_ = m_state_;
		CheckGround();
	}

	void PlayerScript::Update(const float& dt)
	{
		const auto rb = GetOwner().lock()->GetComponent<Components::Rigidbody>().lock();

		if (!rb)
		{
			return;
		}

		if constexpr (g_debug_observer)
		{
			if (const auto head = getHead().lock();
				head && !m_top_view_)
			{
				const auto head_tr = head->GetComponent<Components::Transform>().lock();
				const auto mouse_y = GetMouseManager().GetMouseYRotation();
				head_tr->SetLocalRotation(mouse_y);
			}

			const auto body_tr = GetOwner().lock()->GetComponent<Components::Transform>().lock();
			const auto mouse_x = GetMouseManager().GetMouseXRotation();

			body_tr->SetLocalRotation(mouse_x);
		}

		CheckJump(rb);
		CheckMove(rb);
		CheckAttack(dt);

		switch (GetState())
		{
		case CHAR_STATE_IDLE:
			if (HasStateChanged())
			{
				GetDebugger().Log("Idle");
			}
			break;
		case CHAR_STATE_WALK:
			if (HasStateChanged())
			{
				GetDebugger().Log("Walk");
			}
			break;
		case CHAR_STATE_RUN:
			break;
		case CHAR_STATE_JUMP:
			if (HasStateChanged())
			{
				GetDebugger().Log("Jump");
			}
			break;
		case CHAR_STATE_ATTACK:
			if (HasStateChanged())
			{
				GetDebugger().Log("Attack");
			}
			break;
		case CHAR_STATE_DIE:
			break;
		case CHAR_STATE_HIT:
			break;
		case CHAR_STATE_MAX:
		default:
			break;
		}
	}

	void PlayerScript::PostUpdate(const float& dt) {}

	void PlayerScript::FixedUpdate(const float& dt) {}

	void PlayerScript::PreRender(const float& dt) {}

	void PlayerScript::Render(const float& dt) {}

	void PlayerScript::PostRender(const float& dt) {}

	void PlayerScript::SetActive(bool active)
	{
		MoveCameraToChild(active);
		Script::SetActive(active);
	}

	void PlayerScript::OnImGui()
	{
		Script::OnImGui();
		intDisabled("Current State", m_state_);
		intDisabled("Previous State", m_prev_state_);
		CheckboxAligned("Top View", m_top_view_);
		lldDisabled("Camera ID", m_cam_id_);
		FloatAligned("HP", m_hp_);
	}

	UINT PlayerScript::GetHealth() const
	{
		return m_hp_;
	}

	void PlayerScript::Hit(const float damage)
	{
		m_hp_ -= damage;

		if (m_hp_ <= 0.f)
		{
			SetState(CHAR_STATE_DIE);
		}
		else
		{
			SetState(CHAR_STATE_HIT);
		}
	}

	void PlayerScript::OnCollisionEnter(const WeakCollider& other) {}

	void PlayerScript::OnCollisionContinue(const WeakCollider& other) {}

	void PlayerScript::OnCollisionExit(const WeakCollider& other) {}

	PlayerScript::PlayerScript()
		: m_state_(CHAR_STATE_IDLE),
		  m_prev_state_(CHAR_STATE_IDLE),
		  m_top_view_(false),
		  m_cam_id_(0),
		  m_hp_(100.f),
		  m_shoot_interval_(0) {}

	WeakObjectBase PlayerScript::getHead() const
	{
		if (const auto& owner = GetOwner().lock())
		{
			if (const auto& script = owner->GetScript<PlayerHitboxScript>().lock())
			{
				return script->GetHead();
			}
		}

		return {};
	}

	float PlayerScript::getFireRate() const
	{
		if (const auto owner = GetOwner().lock();
			owner)
		{
			if (const auto script = owner->GetScript<RifleScript>().lock())
			{
				return script->GetFireRate();
			}
		}

		return FLT_MAX;
	}

	void PlayerScript::Hitscan(const float damage, const float range) const
	{
		const auto& head = getHead().lock();

		if (!head)
		{
			return;
		}

		const auto head_tr = head->GetComponent<Components::Transform>().lock();
		const auto owner   = GetOwner().lock();
		const auto lcl     = GetOwner().lock()->GetComponent<Components::Collider>().lock();
		const auto start   = head_tr->GetWorldPosition();
		const auto forward = head_tr->Forward();

		const auto end = start + forward * range;

		if (const auto scene = GetOwner().lock()->GetScene().lock())
		{
			const auto& tree = scene->GetObjectTree();

			std::queue<const Octree*> queue;
			queue.push(&tree);

			while (!queue.empty())
			{
				const auto  node     = queue.front();
				const auto& value    = node->Read();
				const auto& children = node->Next();
				const auto& active   = node->ActiveChildren();
				queue.pop();

				for (const auto& p_rhs : value)
				{
					if (const auto& rhs = p_rhs.lock())
					{
						if (rhs->GetParent().lock() == owner)
						{
							continue;
						}
						if (rhs == owner)
						{
							continue;
						}

						if (const auto& rcl = rhs->GetComponent<Components::Collider>().lock())
						{
							if (rcl->GetActive())
							{
								float dist = 0.f;

								if (rcl->Intersects(start, forward, range, dist))
								{
									if (const auto script = rhs->GetScript<HitboxScript>().lock())
									{
										GetDebugger().Log
												(
												 std::format
												 (
												  "Hit {} for {} damage",
												  rhs->GetName(),
												  damage
												 )
												);

										script->Hit(damage);
									}
									else
									{
										rcl->onCollisionEnter.Broadcast(lcl);
									}
								}
							}
						}
					}
				}

				// Add children to stack.
				for (int i = 7; i >= 0; --i)
				{
					if (children[i] && children[i]->Contains(end))
					{
						queue.push(children[i]);
					}
				}
			}
		}
	}

	eCharacterState PlayerScript::GetState() const
	{
		return m_state_;
	}

	void PlayerScript::SetState(const eCharacterState state)
	{
		m_prev_state_ = m_state_;
		m_state_      = state;
	}

	bool PlayerScript::HasStateChanged() const
	{
		return m_state_ != m_prev_state_;
	}

	void PlayerScript::MoveCameraToChild(bool active)
	{
		if (const auto head  = getHead().lock();
			const auto scene = GetOwner().lock()->GetScene().lock())
		{
			const auto cam = scene->GetMainCamera().lock();

			head->AddChild(cam);
			m_cam_id_ = cam->GetLocalID();
		}
	}

	void PlayerScript::SetHeadView(const bool head_view)
	{
		if (m_cam_id_ == g_invalid_id)
		{
			return;
		}

		const auto head = getHead().lock();

		if (!head)
		{
			return;
		}

		const auto cam_obj = head->GetChild(m_cam_id_).lock();

		if (!cam_obj)
		{
			return;
		}

		const auto cam    = cam_obj->GetSharedPtr<Objects::Camera>();
		const auto cam_tr = cam_obj->GetComponent<Components::Transform>().lock();

		if (m_top_view_)
		{
			cam->SetOrthogonal(false);
			cam_tr->SetLocalPosition({0.f, 10.f, 0.f});
			cam_tr->SetLocalRotation(Quaternion::CreateFromAxisAngle(Vector3::Right, XM_PIDIV2));
		}
		else
		{
			cam->SetOrthogonal(false);
			cam_tr->SetLocalPosition(Vector3::Zero);
			cam_tr->SetLocalRotation(Quaternion::Identity);
		}

		m_top_view_ = head_view;
	}

	void PlayerScript::CheckJump(const boost::shared_ptr<Components::Rigidbody>& rb)
	{
		if (!rb->GetGrounded())
		{
			SetState(CHAR_STATE_JUMP);
		}
		else
		{
			SetState(CHAR_STATE_IDLE);
		}
	}

	void PlayerScript::CheckMove(const boost::shared_ptr<Components::Rigidbody>& rb)
	{
		if constexpr (g_debug_observer)
		{
			return;
		}

		float      speed = 10.0f;
		const auto scene = GetOwner().lock()->GetScene().lock();

		auto    forward = GetOwner().lock()->GetComponent<Components::Transform>().lock()->Forward();
		Vector3 ortho;
		forward.Cross(Vector3::Up, ortho);

		forward *= {1.f, 0.f, 1.f};
		ortho *= {1.f, 0.f, 1.f};

		forward *= speed;
		ortho *= speed;

		bool       pressed = false;
		const auto atr     = GetOwner().lock()->GetComponent<Components::Animator>().lock();

		constexpr UINT forward_anim  = 20;
		constexpr UINT backward_anim = 19;
		constexpr UINT left_anim     = 22;
		constexpr UINT right_anim    = 21;
		constexpr UINT idle_anim     = 0;

		if (GetApplication().GetCurrentKeyState().IsKeyDown(Keyboard::W))
		{
			atr->SetAnimation(forward_anim);
			rb->AddT1Force(forward);
			pressed = true;
		}

		if (GetApplication().GetCurrentKeyState().IsKeyDown(Keyboard::A))
		{
			atr->SetAnimation(left_anim);
			rb->AddT1Force(ortho);
			pressed = true;
		}

		if (GetApplication().GetCurrentKeyState().IsKeyDown(Keyboard::S))
		{
			atr->SetAnimation(backward_anim);
			rb->AddT1Force(-forward);
			pressed = true;
		}

		if (GetApplication().GetCurrentKeyState().IsKeyDown(Keyboard::D))
		{
			atr->SetAnimation(right_anim);
			rb->AddT1Force(-ortho);
			pressed = true;
		}

		if (!pressed)
		{
			SetState(CHAR_STATE_IDLE);
			atr->SetAnimation(idle_anim);
		}
		else
		{
			SetState(CHAR_STATE_WALK);
		}
	}

	void PlayerScript::CheckAttack(const float& dt)
	{
		if constexpr (g_debug_observer)
		{
			return;
		}

		if (GetApplication().GetMouseState().leftButton)
		{
			SetState(CHAR_STATE_ATTACK);

			const auto& fire_rate = getFireRate();

			if (m_shoot_interval_ < fire_rate)
			{
				m_shoot_interval_ += dt;
				return;
			}

			m_shoot_interval_ = 0.f;
			const auto tr     =
					GetOwner().lock()->GetComponent<Components::Transform>().lock();

			Ray ray;
			ray.position  = tr->GetWorldPosition();
			ray.direction = tr->Forward();

			constexpr float distance = 5.f;

			GetDebugger().Draw(ray, Colors::AliceBlue);
			std::vector<WeakObjectBase> out;

			Hitscan(10.f, 10.f);
		}
	}

	void PlayerScript::CheckGround() const
	{
		const auto  scene = GetOwner().lock()->GetScene().lock();
		const auto& tree  = scene->GetObjectTree();
		const auto  rb    = GetOwner().lock()->GetComponent<Components::Rigidbody>().lock();

		std::queue<const Octree*> q;
		q.push(&tree);

		while (!q.empty())
		{
			const auto node = q.front();
			q.pop();

			const auto& value    = node->Read();
			const auto& children = node->Next();

			for (const auto v : value)
			{
				const auto lcl = GetOwner().lock()->GetComponent<Components::Collider>().lock();
				const auto rcl = v.lock()->GetComponent<Components::Collider>().lock();

				if (!GetCollisionDetector().IsCollisionLayer(GetOwner().lock()->GetLayer(), v.lock()->GetLayer()))
				{
					continue;
				}
				if (!rcl || lcl == rcl)
				{
					continue;
				}

				const auto owner        = rcl->GetOwner().lock();
				const auto owner_parent = owner->GetParent();

				if (owner_parent.lock() == GetOwner().lock())
				{
					continue;
				}

				if (Components::Collider::Intersects(lcl, rcl, Vector3::Down))
				{
					rb->SetGrounded(true);
					return;
				}
			}

			for (const auto& child : children)
			{
				if (child && child->Contains
				    (
				     GetOwner().lock()->GetComponent<Components::Transform>().lock()->GetWorldPosition()
				    ))
				{
					q.push(child);
				}
			}
		}
	}
}
