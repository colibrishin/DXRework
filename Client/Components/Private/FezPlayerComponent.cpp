#include "Components/Public/FezPlayerComponent.h"
#include "FezPlayerComponent.generated.h"

#include "MathExtension.hpp"
#include "Rigidbody.h"
#include "ObjectBase.h"
#include "Transform.h"
#include "Camera.h"
#include "InputManager.h"
#include "Components/Public/CubifyComponent.h"
#include "Collider.h"
#include "CollisionDetector.h"

using namespace Engine;

const Quaternion FezPlayerComponent::s_cw_rotations[4] =
{
	Quaternion::CreateFromAxisAngle(FezPlayerComponent::s_up, 0.0f),
	Quaternion::CreateFromAxisAngle(FezPlayerComponent::s_up, -MathExtension::ToRadian(90.f)),
	Quaternion::CreateFromAxisAngle(FezPlayerComponent::s_up, -MathExtension::ToRadian(180.f)),
	Quaternion::CreateFromAxisAngle(FezPlayerComponent::s_up, -MathExtension::ToRadian(270.f))
};

inline FezPlayerComponent::FezPlayerComponent(const Engine::Weak<Engine::Abstracts::ObjectBase>& owner) :
	Component(owner),
	m_state_(CHAR_STATE_IDLE),
	m_prev_state_(CHAR_STATE_IDLE),
	m_b_grounded_(false),
	m_rotation_count_(0),
	m_accumulated_dt_(0),
	m_rotate_allowed_(true),
	m_rotate_finished_(false),
	m_rotate_consecutive_(false),
	m_b_climbing_(false),
	m_b_vaulting_(false) { }

void FezPlayerComponent::Initialize()
{
	Component::Initialize();
}

void FezPlayerComponent::BeginPlay( const float dt )
{
    Component::BeginPlay( dt );
    MoveCameraToChild();

    const auto &owner = GetOwner().lock();
    if ( !owner )
    {
        return;
    }

    const auto &rb = owner->GetComponent<Components::Rigidbody>().lock();
    if ( !rb )
    {
        return;
    }

    rb->SetFrictionCoefficient( 0.1 );
}

void FezPlayerComponent::PreUpdate(const float dt)
{
}

void FezPlayerComponent::Update(const float dt)
{
	m_prev_state_ = m_state_;

	UpdateGrounded();

	switch (m_state_)
	{
	case CHAR_STATE_IDLE:
	case CHAR_STATE_WALK:
		UpdateRotate(dt);
		UpdateMove();
		UpdateInitialJump();
		UpdateInitialClimb();
		UpdateInitialVault();
		break;
	case CHAR_STATE_RUN:
		break;
	case CHAR_STATE_JUMP:
		UpdateMove();
		UpdateJump();
		UpdateInitialClimb();
		break;
	case CHAR_STATE_CLIMB:
		UpdateClimb();
		UpdateRotate(dt);
		break;
	case CHAR_STATE_SWIM:
		break;
	case CHAR_STATE_ROTATE:
		UpdateRotate(dt);
		break;
	case CHAR_STATE_POST_ROTATE:
		if (m_state_ == CHAR_STATE_POST_ROTATE &&
			m_prev_state_ == CHAR_STATE_POST_ROTATE &&
			m_rotate_finished_)
		{
			if (m_b_vaulting_)
			{
				UpdateVault();
				UpdateInitialJump();
			}
			else if (m_b_climbing_)
			{
				UpdateClimb();
				UpdateInitialJump();
			}
			else
			{
				UpdateMove();
				UpdateInitialJump();
			}
		}
		UpdateRotate(dt);
		break;
	case CHAR_STATE_FALL:
		UpdateMove();
		UpdateFall();
		break;
	case CHAR_STATE_ATTACK:
		break;
	case CHAR_STATE_HIT:
		break;
	case CHAR_STATE_DIE:
		break;
	case CHAR_STATE_MAX:
		break;
	case CHAR_STATE_POST_CLIMB:
		break;
	case CHAR_STATE_VAULT:
		UpdateVault();
		UpdateRotate(dt);
		UpdateInitialJump();
		break;
	default:;
	}
}

void FezPlayerComponent::PostUpdate(const float dt)
{
}

void FezPlayerComponent::FixedUpdate(const float dt)
{
}

Vector3 FezPlayerComponent::GetForward() const
{
	return Vector3::Transform(g_forward, s_cw_rotations[m_rotation_count_]);
}

bool FezPlayerComponent::IsVisible(const Engine::Weak<Engine::Components::Transform>& otr) const
{
	if (const auto& locked = otr.lock())
	{
		const auto& owner = GetOwner().lock();
		if (!owner)
		{
			return false;
		}

		const auto& tr = owner->GetComponent<Components::Transform>().lock();
		if (!tr)
		{
			return false;
		}

		const auto& pos = locked->GetWorldPosition();
		const auto& forward = GetForward();
		const auto& obj_forward = locked->Forward();
		const auto& dot = forward.Dot(obj_forward);

		return !MathExtension::FloatCompare(dot, -1.f);
	}

	return false;
}

FezPlayerComponent::FezPlayerComponent() : 
	Component({}),
	m_state_(CHAR_STATE_IDLE),
	m_prev_state_(CHAR_STATE_IDLE),
	m_b_grounded_(false),
	m_rotation_count_(0),
	m_accumulated_dt_(0),
	m_rotate_allowed_(true),
	m_rotate_finished_(false),
	m_rotate_consecutive_(false),
	m_b_climbing_(false),
	m_b_vaulting_(false)
{
}

void FezPlayerComponent::IgnoreCollision() const
{
	if (const auto& owner = GetOwner().lock())
	{
		if (const auto& scene = owner->GetScene().lock())
		{
			scene->ChangeLayer(RESERVED_LAYER_OBSERVER, owner->GetID());
		}
	}
}

void FezPlayerComponent::ApplyCollision() const
{
	if (const auto& owner = GetOwner().lock())
	{
		if (const auto& scene = owner->GetScene().lock())
		{
			scene->ChangeLayer(RESERVED_LAYER_DEFAULT, owner->GetID());
		}
	}
}

void FezPlayerComponent::IgnoreGravity() const
{
	if (const auto& owner = GetOwner().lock())
	{
		if (const auto& rb = owner->GetComponent<Components::Rigidbody>().lock())
		{
			rb->SetGravityOverride(false);
		}
	}
}

void FezPlayerComponent::ApplyGravity() const
{
	if (const auto& owner = GetOwner().lock())
	{
		if (const auto& rb = owner->GetComponent<Components::Rigidbody>().lock())
		{
			rb->SetFixed(true);
		}
	}
}

void FezPlayerComponent::IgnoreLerp() const
{
	if (const auto& owner = GetOwner().lock())
	{
		if (const auto& rb = owner->GetComponent<Components::Rigidbody>().lock())
		{
			rb->SetFixed(true);
		}
	}
}

void FezPlayerComponent::ApplyLerp() const
{
	if (const auto& owner = GetOwner().lock())
	{
		if (const auto& rb = owner->GetComponent<Components::Rigidbody>().lock())
		{
			rb->SetFixed(false);
		}
	}
}

void FezPlayerComponent::Fullstop() const
{
	if (const auto& owner = GetOwner().lock())
	{
		if (const auto& rb = owner->GetComponent<Components::Rigidbody>().lock())
		{
			rb->FullReset();
		}
	}
}

void FezPlayerComponent::MoveCameraToChild() const
{
	if (const auto& owner = GetOwner().lock())
	{
		if (const auto& rb = owner->GetComponent<Components::Rigidbody>().lock())
		{
			rb->SetNoAngular(true);
		}

		if (const auto& scene = owner->GetScene().lock())
		{
			scene->SetMainActor(owner->GetLocalID());

			if (const auto& cam = scene->GetMainCamera().lock())
			{
				owner->AddChild(cam->GetSharedPtr<Abstracts::ObjectBase>());
				cam->SetName("Camera");
				cam->SetFOV(20.f);
				cam->SetOrthogonal(true);

				if (const auto& tr = cam->GetComponent<Components::Transform>().lock())
				{
					tr->SetLocalPosition({ 0.0f, 0.0f, -10.0f });
				}
			}
		}
	}
}

void FezPlayerComponent::UpdateMove()
{
	if (GetOwner().expired())
	{
		return;
	}

	const auto& owner = GetOwner().lock();
	const auto& tr = owner->GetComponent<Components::Transform>().lock();
	const auto& rb = owner->GetComponent<Components::Rigidbody>().lock();
	const auto& cam = owner->GetChild("Camera").lock();

	if (!tr || !rb || !cam)
	{
		return;
	}
	if (!owner->GetActive() || !tr->GetActive() || !rb->GetActive() || !cam->GetActive())
	{
		return;
	}

	const auto& right = tr->Right();
	constexpr float speed = 10.f;
	bool            moving = false;

	// todo: speed limit
	if (Managers::InputManager::GetInstance().IsKeyPressed(Keyboard::D))
	{
		if (rb->GetT0LinearVelocity().x <= speed)
		{
			rb->AddT1Force(right * speed);
			moving = true;
		}
	}
	if (Managers::InputManager::GetInstance().IsKeyPressed(Keyboard::A))
	{
		if (rb->GetT0LinearVelocity().x >= -speed)
		{
			rb->AddT1Force(-right * speed);
			moving = true;
		}
	}

	if (moving && (m_state_ == CHAR_STATE_IDLE || m_state_ == CHAR_STATE_POST_ROTATE))
	{
		m_state_ = CHAR_STATE_WALK;
	}
}

void FezPlayerComponent::UpdateRotate(float dt)
{
	if (GetOwner().expired())
	{
		return;
	}

	const auto& owner = GetOwner().lock();
	const auto& tr = owner->GetComponent<Components::Transform>().lock();
	const auto& rb = owner->GetComponent<Components::Rigidbody>().lock();
	const auto& cldr = owner->GetComponent<Components::Collider>().lock();

	if (!tr || !rb || !cldr)
	{
		return;
	}
	if (!owner->GetActive() || !tr->GetActive() || !rb->GetActive())
	{
		return;
	}

	bool rotating = false;

	// If the player is not allowed to rotate.
	if (!m_rotate_allowed_)
	{
		return;
	}

	// Case where the player is rotating.
	if (m_state_ == CHAR_STATE_ROTATE)
	{
		// Wait for the rotation to be close to the target rotation.
		// todo: lerp rotation speed between 0 to 1
		if (s_rotation_speed > m_accumulated_dt_)
		{
			const auto rot = tr->GetLocalRotation();
			tr->SetLocalRotation(Quaternion::Slerp(rot, s_cw_rotations[m_rotation_count_], m_accumulated_dt_));
			m_accumulated_dt_ += dt;
			return;
		}
		// If the player is rotating and rotation is completed,
		// Set the player's state to post rotate
		m_state_ = CHAR_STATE_POST_ROTATE;
		return;
	}
	if (m_prev_state_ == CHAR_STATE_POST_ROTATE &&
		m_state_ == CHAR_STATE_POST_ROTATE &&
		!m_rotate_finished_)
	{
		// Set the rotation to the accurate target rotation.
		tr->SetLocalRotation(s_cw_rotations[m_rotation_count_]);
		m_accumulated_dt_ = 0.f;

		// Player rotation is finished.
		m_rotate_finished_ = true;
		return;
	}
	if (m_prev_state_ == CHAR_STATE_POST_ROTATE &&
		m_state_ != CHAR_STATE_POST_ROTATE &&
		m_state_ != CHAR_STATE_ROTATE &&
		m_rotate_finished_)
	{
		// Clear accumulated forces (e.g., collision reaction force) and set fixed to false
		Fullstop();
		ApplyLerp();
		m_rotate_finished_ = false;
		m_rotate_consecutive_ = false;

		return;
	}

	// CW
	if (Managers::InputManager::GetInstance().IsKeyPressed(Keyboard::Q))
	{
		m_rotation_count_ = (m_rotation_count_ + 3) % 4;
		rotating = true;
	}

	// CCW
	if (Managers::InputManager::GetInstance().IsKeyPressed(Keyboard::E))
	{
		m_rotation_count_ = (m_rotation_count_ + 1) % 4;
		rotating = true;
	}

	// If the player starts rotating, then set the player's state to rotate.
	// Make the player full stop.
	if (rotating)
	{
		if (m_state_ == CHAR_STATE_POST_ROTATE)
		{
			m_rotate_consecutive_ = true;
		}
		else
		{
			m_rotate_consecutive_ = false;
		}
		m_state_ = CHAR_STATE_ROTATE;

		if (!m_rotate_consecutive_)
		{
			m_last_spin_position_ = tr->GetWorldPosition();
		}

		const auto& scene = owner->GetScene().lock();
		const auto& octree = scene->GetObjectTree();
		if (!scene)
		{
			return;
		}

		Strong<CubifyComponent> ladder;

		if (m_b_climbing_)
		{
			for (const auto& id : cldr->GetCollidedObjects())
			{
				const auto& obj = scene->FindGameObject(id).lock();
				if (!obj)
				{
					continue;
				}

				if (const auto& parent = obj->GetParent().lock())
				{
					ladder = parent->GetComponent<CubifyComponent>().lock();
				}
				else
				{
					ladder = obj->GetComponent<CubifyComponent>().lock();
				}

				if (!ladder)
				{
					continue;
				}
				if (ladder->GetCubeType() != CUBE_TYPE_LADDER)
				{
					continue;
				}
				break;
			}
		}

		CubifyComponent::DispatchNormalUpdate();

		// Find the ground and ask for other cubes whether the player can stand on.
		for (const auto& nearest = octree.Nearest(m_last_spin_position_, 1.5f);
			const auto & obj : nearest)
		{
			const auto& candidate = obj.lock();
			if (!candidate)
			{
				continue;
			}

			Strong<CubifyComponent> script;

			if (const auto& parent = candidate->GetParent().lock())
			{
				script = parent->GetComponent<CubifyComponent>().lock();
			}
			else
			{
				script = candidate->GetComponent<CubifyComponent>().lock();
			}

			if (!script)
			{
				continue;
			}

			const auto& player_pos = m_last_spin_position_;

			if (script->GetCubeType() == CUBE_TYPE_NORMAL)
			{
				if (movePlayerToNearestCube(tr, script, player_pos))
				{
					CubifyComponent::DispatchUpdateWithoutNormal();
					break;
				}
			}
		}

		if (m_b_climbing_)
		{
			if (ladder)
			{
				movePlayerToNearestCube(tr, ladder, tr->GetWorldPosition());
			}
		}

		Fullstop();
		IgnoreLerp();

		m_rotate_finished_ = false;
	}
}

void FezPlayerComponent::UpdateGrounded()
{
	if (GetOwner().expired())
	{
		return;
	}

	const auto& owner = GetOwner().lock();
	const auto& scene = owner->GetScene().lock();
	const auto& tr = owner->GetComponent<Components::Transform>().lock();
	const auto& rb = owner->GetComponent<Components::Rigidbody>().lock();
	const auto& cldr = owner->GetComponent<Components::Collider>().lock();

	if (!tr || !rb || !scene || !cldr)
	{
		return;
	}
	if (!owner->GetActive() || !tr->GetActive() || !rb->GetActive() || !cldr->GetActive())
	{
		return;
	}

	const auto& up = tr->Up();
	const auto& center = tr->GetWorldPosition();
	const auto& down = -up;
	const auto& pos = tr->GetLocalPosition();
	bool        hit = false;

	const auto& octree = scene->GetObjectTree();
	octree.Iterate
	(
		pos, [&hit, &owner, &cldr, &center](const Weak<Abstracts::ObjectBase>& obj)
		{
			if (const auto& locked = obj.lock())
			{
				if (locked == owner)
				{
					return false;
				}
				if (!Managers::CollisionDetector::GetInstance().IsCollisionLayer(owner->GetLayer(), locked->GetLayer()))
				{
					return false;
				}

				const auto& rcl = locked->GetComponent<Components::Collider>().lock();
				const auto& rtr = locked->GetComponent<Components::Transform>().lock();

				if (!rcl || !rtr)
				{
					return false;
				}
				if (!rcl->GetActive() || !rtr->GetActive())
				{
					return false;
				}

				const auto& rowner = rcl->GetOwner().lock();
				if (!rowner)
				{
					return false;
				}
				if (const auto& parent = owner->GetParent().lock();
					rowner == parent)
				{
					return false;
				}

				// Check whether two objects are colliding in direction of down
				// Also check for position in y-axis so that it doesn't collide with ceiling
				if (Components::Collider::Intersects(cldr, rcl, Vector3::Down) &&
					center.y > rtr->GetWorldPosition().y)
				{
					hit = true;
					return true;
				}
			}

			return false;
		}
	);

	if (hit)
	{
		m_b_grounded_ = true;
	}
}

void FezPlayerComponent::UpdateInitialJump()
{
	if (GetOwner().expired())
	{
		return;
	}

	const auto& owner = GetOwner().lock();
	const auto& tr = owner->GetComponent<Components::Transform>().lock();
	const auto& rb = owner->GetComponent<Components::Rigidbody>().lock();

	if (!tr || !rb)
	{
		return;
	}
	if (!owner->GetActive() || !tr->GetActive() || !rb->GetActive())
	{
		return;
	}

	const auto& up = tr->Up();
	const auto& scene = owner->GetScene().lock();

	// Discrete key check for initial jump. Continuous key check for jump will be done in jump state.
	if (Managers::InputManager::GetInstance().IsKeyPressed(Keyboard::Space))
	{
		DoInitialJump(rb, up);
	}
}

void FezPlayerComponent::UpdateJump()
{
	if (GetOwner().expired())
	{
		return;
	}

	const auto& owner = GetOwner().lock();
	const auto& tr = owner->GetComponent<Components::Transform>().lock();
	const auto& rb = owner->GetComponent<Components::Rigidbody>().lock();

	if (!tr || !rb)
	{
		return;
	}
	if (!owner->GetActive() || !tr->GetActive() || !rb->GetActive())
	{
		return;
	}

	const auto& up = tr->Up();
	const auto& scene = owner->GetScene().lock();

	if (Managers::InputManager::GetInstance().IsKeyPressed(Keyboard::W) ||
		Managers::InputManager::GetInstance().IsKeyPressed(Keyboard::Space))
	{
		rb->AddT1Force(up * s_jump_speed);

		if (rb->GetT0LinearVelocity().y >= s_jump_apex)
		{
			m_state_ = CHAR_STATE_FALL;
			ApplyCollision();
		}
	}
	else
	{
		m_state_ = CHAR_STATE_FALL;
		ApplyCollision();
	}
}

void FezPlayerComponent::UpdateFall()
{
	if (GetOwner().expired())
	{
		return;
	}

	const auto& owner = GetOwner().lock();
	const auto& tr = owner->GetComponent<Components::Transform>().lock();
	const auto& rb = owner->GetComponent<Components::Rigidbody>().lock();

	if (!tr || !rb)
	{
		return;
	}
	if (!owner->GetActive() || !tr->GetActive() || !rb->GetActive())
	{
		return;
	}

	if (m_b_grounded_)
	{
		m_state_ = CHAR_STATE_IDLE;
	}
}

void FezPlayerComponent::UpdateInitialClimb()
{
}

void FezPlayerComponent::UpdateClimb()
{
}

void FezPlayerComponent::UpdateInitialVault()
{
	if (GetOwner().expired())
	{
		return;
	}

	const auto& owner = GetOwner().lock();
	const auto& tr = owner->GetComponent<Components::Transform>().lock();
	const auto& rb = owner->GetComponent<Components::Rigidbody>().lock();
	const auto& cldr = owner->GetComponent<Components::Collider>().lock();

	if (!tr || !rb || !cldr)
	{
		return;
	}
	if (!owner->GetActive() || !tr->GetActive() || !rb->GetActive())
	{
		return;
	}

	const auto& scene = owner->GetScene().lock();

	// Check state change just in case, whether the other check succeeds before vault check.
	if (Managers::InputManager::GetInstance().IsKeyPressed(Keyboard::S) &&
		(m_state_ == CHAR_STATE_IDLE || m_state_ == CHAR_STATE_WALK))
	{
		doDownVault(tr);
		m_b_vaulting_ = true;
		return;
	}

	// Check whether caught the cube, if so, set the player's state to vault.
	// Use the HasKeyChanged to avoid the easy vaulting by pressing the key.
	if (Managers::InputManager::GetInstance().IsKeyPressed(Keyboard::W) &&
		(m_state_ == CHAR_STATE_JUMP || m_state_ == CHAR_STATE_FALL))
	{
		for (const auto& id : cldr->GetCollidedObjects())
		{
			const auto& candidate = scene->FindGameObject(id).lock();
			if (!candidate)
			{
				continue;
			}

			const auto& script = candidate->GetComponent<CubifyComponent>().lock();
			if (!script)
			{
				continue;
			}

			if (const auto& nearest_cube = script->GetDepthNearestCube(tr->GetWorldPosition()).lock())
			{
				const auto& ntr = nearest_cube->GetComponent<Components::Transform>().lock();
				const auto& cube_pos = ntr->GetWorldPosition();
				const auto& player_pos = tr->GetWorldPosition();
				const auto& new_pos = Vector3
				{
					m_rotation_count_ == 1 || m_rotation_count_ == 3 ? cube_pos.x : player_pos.x,
					player_pos.y,
					m_rotation_count_ == 0 || m_rotation_count_ == 2 ? cube_pos.z : player_pos.z
				};

				tr->SetWorldPosition(new_pos);
				IgnoreGravity();
				Fullstop();

				// Change the layer to default.
				IgnoreCollision();

				m_state_ = CHAR_STATE_VAULT;
				m_b_vaulting_ = true;
				return;
			}
		}
	}
}

void FezPlayerComponent::UpdateVault()
{
	if (GetOwner().expired())
	{
		return;
	}

	const auto& owner = GetOwner().lock();
	const auto& tr = owner->GetComponent<Components::Transform>().lock();
	const auto& rb = owner->GetComponent<Components::Rigidbody>().lock();
	const auto& cldr = owner->GetComponent<Components::Collider>().lock();

	if (!tr || !rb || !cldr)
	{
		return;
	}
	if (!owner->GetActive() || !tr->GetActive() || !rb->GetActive() || !cldr->GetActive())
	{
		return;
	}

	const auto& up = tr->Up();
	const auto& down = -up;
	const auto& right = tr->Right();
	const auto& left = -right;

	const auto& scene = owner->GetScene().lock();

	const auto& pos = tr->GetLocalPosition();

	bool pressed = false;

	// todo: slight overhead.
	if (rb->IsFixed())
	{
		// Revert the fixed state.
		ApplyLerp();
	}

	// Moving while in vault state.
	if (Managers::InputManager::GetInstance().IsKeyDown(Keyboard::D))
	{
		pressed = true;
		rb->AddT1Force(right * 1.f);
	}
	if (Managers::InputManager::GetInstance().IsKeyDown(Keyboard::A))
	{
		pressed = true;
		rb->AddT1Force(left * 1.f);
	}

	// If the player is previously vaulted, revert back to the vault state.
	if (m_state_ == CHAR_STATE_POST_ROTATE && pressed && m_rotate_finished_)
	{
		m_state_ = CHAR_STATE_VAULT;
	}

	// Vaulting up.
	if (Managers::InputManager::GetInstance().IsKeyPressed(Keyboard::W))
	{
		const auto& size = tr->GetLocalScale();
		tr->SetLocalPosition(pos + Vector3{ 0.f, size.y / 2, 0.f });
		ApplyGravity();
		Fullstop();

		// Revert the layer to default.
		ApplyCollision();

		m_state_ = CHAR_STATE_IDLE;
		return;
	}

	// Vaulting down. (falling)
	if (Managers::InputManager::GetInstance().IsKeyPressed(Keyboard::S))
	{
		ApplyGravity();
		Fullstop();

		// todo: Move the player to the down position of the nearest ground.
		for (const auto& id : cldr->GetCollidedObjects())
		{
			const auto& candidate = scene->FindGameObject(id).lock();
			if (!candidate)
			{
				continue;
			}

			const auto& script = candidate->GetComponent<CubifyComponent>().lock();
			if (!script)
			{
				continue;
			}

			if (const auto& nearest_cube = script->GetDepthNearestCube(tr->GetWorldPosition()).lock())
			{
				const auto& ntr = nearest_cube->GetComponent<Components::Transform>().lock();
				const auto& cube_size = ntr->GetLocalScale();
				const auto& cube_pos = ntr->GetWorldPosition();
				const auto& player_size = tr->GetLocalScale();
				const auto& player_pos = tr->GetWorldPosition();
				const auto& new_pos = Vector3
				{
					player_pos.x,
					cube_pos.y - (cube_size.y / 2) - (player_size.y / 2),
					player_pos.z
				};

				tr->SetWorldPosition(new_pos);
				break;
			}
		}

		// Revert the layer to default.
		ApplyCollision();

		m_state_ = CHAR_STATE_FALL;
	}
}

void FezPlayerComponent::DoInitialJump(const Engine::Strong<Engine::Components::Rigidbody>& rb, const Vector3& up)
{
	rb->AddT1Force(up * s_jump_initial_speed);
	m_state_ = CHAR_STATE_JUMP;
	// Change the layer to none so that the player can jump through the cube.
	IgnoreCollision();
}

bool FezPlayerComponent::doInitialClimb(const Engine::Strong<Engine::Components::Transform>& tr, const Vector3& pos, const Engine::Weak<Engine::Abstracts::ObjectBase>& obj, bool& continues)
{
	continues = false;

	if (const auto& locked = obj.lock())
	{
		const auto& parent = locked->GetParent().lock();
		Strong<CubifyComponent> script;

		if (!parent)
		{
			script = locked->GetComponent<CubifyComponent>().lock();
		}
		else
		{
			script = parent->GetComponent<CubifyComponent>().lock();
		}

		if (script)
		{
			if (script->GetCubeType() != CUBE_TYPE_LADDER)
			{
				continues = true;
				return false;
			}

			if (const auto& nearest_cube = script->GetDepthNearestCube(pos).lock())
			{
				const auto& ntr = nearest_cube->GetComponent<Components::Transform>().lock();
				const auto& cube_pos = ntr->GetWorldPosition();

				const auto& new_pos = Vector3
				{
					m_rotation_count_ == 1 || m_rotation_count_ == 3 ? cube_pos.x : pos.x,
					pos.y,
					m_rotation_count_ == 0 || m_rotation_count_ == 2 ? cube_pos.z : pos.z
				};

				tr->SetWorldPosition(new_pos);
				IgnoreGravity();

				m_state_ = CHAR_STATE_CLIMB;
				m_b_climbing_ = true;
				return true;
			}
		}
	}
	return false;
}

bool FezPlayerComponent::movePlayerToNearestCube(const Engine::Strong<Engine::Components::Transform>& tr, const Engine::Strong<CubifyComponent>& component, const Vector3& player_pos) const
{
	// Active nearest cube should be the one that the player can stand on.
	if (const auto& near_cube = component->GetDepthNearestCube(player_pos).lock())
	{
		const auto& ntr = near_cube->GetComponent<Components::Transform>().lock();
		const auto& cube_pos = ntr->GetWorldPosition();
		const auto& new_pos = Vector3
		{
			cube_pos.x,
			player_pos.y,
			cube_pos.z
		};

		tr->SetWorldPosition(new_pos);
		return true;
	}

	return false;
}

void FezPlayerComponent::doDownVault(const Engine::Strong<Engine::Components::Transform>& tr)
{
	tr->SetLocalPosition(tr->GetLocalPosition() - (tr->GetLocalScale() * 0.5f));

	// Set the player to fixed for avoiding the lerp.
	IgnoreLerp();
	IgnoreGravity();

	// Reset the rigidbody for avoiding the player slipping down.
	Fullstop();

	// Change the layer to none for ignoring the collision.
	IgnoreCollision();

	m_state_ = CHAR_STATE_VAULT;
}

void FezPlayerComponent::OnSerialized()
{
}

void FezPlayerComponent::OnDeserialized()
{
}

eComponentUpdatePriorities FezPlayerComponent::GetUpdatePriority() const
{
	return eComponentUpdatePriority::COM_PRIORITY_POSITIONAL;
}

#if WITH_EDITOR
void FezPlayerComponent::OnUIUpdate(Engine::UIContext* const parent, const float dt)
{
	if (parent)
	{
		static constexpr auto state_enum = CStrEnumStrings<eCharacterState>();
		Component::OnUIUpdate(parent, dt);
		IUIAPI& ui = s_uia.GetInterface();
        *parent |= ui.NewCombobox( this,
                                   "PreviousState",
                                   { "Previous State",
                                     reinterpret_cast<int *>( &m_prev_state_ ),
                                     state_enum.data(),
                                     state_enum.size(),
                                     false } );
        *parent |= ui.NewCombobox( this,
                                   "CurrentState",
                                   { "Current State",
                                     reinterpret_cast<int *>( &m_state_ ),
                                     state_enum.data(),
                                     state_enum.size(),
                                     false } );
	}
}
#endif
