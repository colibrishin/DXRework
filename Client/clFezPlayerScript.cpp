#include "pch.h"
#include "clFezPlayerScript.h"

#include "clCubifyScript.h"
#include "egBaseCollider.hpp"
#include "egCamera.h"
#include "egCollisionDetector.h"
#include "egObjectBase.hpp"
#include "egRigidbody.h"
#include "egTransform.h"

SERIALIZE_IMPL
(
 Client::Scripts::FezPlayerScript,
 _ARTAG(_BSTSUPER(Engine::Script))
 _ARTAG(m_state_)
 _ARTAG(m_prev_state_)
 _ARTAG(m_rotation_count_)
 _ARTAG(m_rotate_allowed_)
)

namespace Client::Scripts
{
	void FezPlayerScript::MoveCameraToChild() const
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
					owner->AddChild(cam->GetSharedPtr<Abstract::ObjectBase>());
					cam->SetName("Camera");
					cam->SetFOV(20.f);
					cam->SetOrthogonal(true);

					if (const auto& tr = cam->GetComponent<Components::Transform>().lock())
					{
						tr->SetLocalPosition({0.0f, 0.0f, -10.0f});
					}
				}
			}
		}
	}

	void FezPlayerScript::Initialize()
	{
		Script::Initialize();
		MoveCameraToChild();

		const auto& owner = GetOwner().lock();
		if (!owner)
		{
			return;
		}

		const auto& rb = owner->GetComponent<Components::Rigidbody>().lock();
		if (!rb)
		{
			return;
		}

		rb->SetFrictionCoefficient(0.1);
	}

	void FezPlayerScript::PreUpdate(const float& dt) {}

	void FezPlayerScript::Update(const float& dt)
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
		default: ;
		}
	}

	void FezPlayerScript::PostUpdate(const float& dt) {}

	void FezPlayerScript::FixedUpdate(const float& dt) {}

	void FezPlayerScript::PreRender(const float& dt) {}

	void FezPlayerScript::Render(const float& dt) {}

	void FezPlayerScript::PostRender(const float& dt) {}

	Vector3 FezPlayerScript::GetForward() const
	{
		return Vector3::Transform(g_forward, s_cw_rotations[m_rotation_count_]);
	}

	bool FezPlayerScript::IsVisible(const WeakTransform& otr) const
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

			const auto& pos         = locked->GetWorldPosition();
			const auto& forward     = GetForward();
			const auto& obj_forward = locked->Forward();
			const auto& dot         = forward.Dot(obj_forward);

			return !FloatCompare(dot, -1.f);
		}

		return false;
	}

	void FezPlayerScript::OnImGui()
	{
		Script::OnImGui();
		constexpr const char* states[]
		{
			"Idle", "Walk", "Run", "Jump", "Climb", "Swim", "Rotate", "Post Rotate",
			"Fall", "Attack", "Hit", "Die", "Max"
		};

		ImGui::BeginDisabled();
		ImGui::Combo("State", &reinterpret_cast<int&>(m_state_), states, std::size(states));
		ImGui::Combo("Prev State", &reinterpret_cast<int&>(m_prev_state_), states, std::size(states));
		ImGui::EndDisabled();
	}

	void FezPlayerScript::OnCollisionEnter(const WeakCollider& other) {}

	void FezPlayerScript::OnCollisionContinue(const WeakCollider& other) {}

	void FezPlayerScript::OnCollisionExit(const WeakCollider& other) {}

	SCRIPT_CLONE_IMPL(FezPlayerScript)

	FezPlayerScript::FezPlayerScript()
		: Script(SCRIPT_T_FEZ_PLAYER, {}),
		  m_state_(CHAR_STATE_IDLE),
		  m_prev_state_(CHAR_STATE_IDLE),
		  m_rotation_count_(0),
		  m_accumulated_dt_(0),
		  m_rotate_allowed_(true),
		  m_rotate_finished_(false),
		  m_rotate_consecutive_(false),
		  m_b_climbing_(false),
		  m_b_vaulting_(false) { }

	void FezPlayerScript::IgnoreCollision() const
	{
		if (const auto& owner = GetOwner().lock())
		{
			if (const auto& scene = owner->GetScene().lock())
			{
				scene->ChangeLayer(LAYER_NONE, owner->GetID());
			}
		}
	}

	void FezPlayerScript::ApplyCollision() const
	{
		if (const auto& owner = GetOwner().lock())
		{
			if (const auto& scene = owner->GetScene().lock())
			{
				scene->ChangeLayer(LAYER_PLAYER, owner->GetID());
			}
		}
	}

	void FezPlayerScript::Fullstop() const
	{
		if (const auto& owner = GetOwner().lock())
		{
			if (const auto& rb = owner->GetComponent<Components::Rigidbody>().lock())
			{
				rb->FullReset();
			}
		}
	}

	void FezPlayerScript::IgnoreGravity() const
	{
		if (const auto& owner = GetOwner().lock())
		{
			if (const auto& rb = owner->GetComponent<Components::Rigidbody>().lock())
			{
				rb->SetGravityOverride(false);
			}
		}
	}

	void FezPlayerScript::ApplyGravity() const
	{
		if (const auto& owner = GetOwner().lock())
		{
			if (const auto& rb = owner->GetComponent<Components::Rigidbody>().lock())
			{
				rb->SetGravityOverride(true);
			}
		}
	}

	void FezPlayerScript::IgnoreLerp() const
	{
		if (const auto& owner = GetOwner().lock())
		{
			if (const auto& rb = owner->GetComponent<Components::Rigidbody>().lock())
			{
				rb->SetFixed(true);
			}
		}
	}

	void FezPlayerScript::ApplyLerp() const
	{
		if (const auto& owner = GetOwner().lock())
		{
			if (const auto& rb = owner->GetComponent<Components::Rigidbody>().lock())
			{
				rb->SetFixed(false);
			}
		}
	}

	void FezPlayerScript::UpdateMove()
	{
		if (GetOwner().expired())
		{
			return;
		}

		const auto& owner = GetOwner().lock();
		const auto& tr    = owner->GetComponent<Components::Transform>().lock();
		const auto& rb    = owner->GetComponent<Components::Rigidbody>().lock();
		const auto& cam   = owner->GetChild("Camera").lock();

		if (!tr || !rb || !cam)
		{
			return;
		}
		if (!owner->GetActive() || !tr->GetActive() || !rb->GetActive() || !cam->GetActive())
		{
			return;
		}

		const auto&     right  = tr->Right();
		constexpr float speed  = 10.f;
		bool            moving = false;

		// todo: speed limit
		if (GetApplication().IsKeyPressed(Keyboard::D))
		{
			if (rb->GetT0LinearVelocity().x <= speed)
			{
				rb->AddT1Force(right * speed);
				moving = true;
			}
		}
		if (GetApplication().IsKeyPressed(Keyboard::A))
		{
			if (rb->GetT0LinearVelocity().x >= -speed)
			{
				rb->AddT1Force(-right * speed);
				moving = true;
			}
		}

		if (moving &&
		    (m_state_ == CHAR_STATE_IDLE ||
		     m_state_ == CHAR_STATE_POST_ROTATE))
		{
			m_state_ = CHAR_STATE_WALK;
		}
	}

	bool FezPlayerScript::movePlayerToNearestCube(
		const StrongTransform& tr, const boost::shared_ptr<CubifyScript>& script, const Vector3& player_pos
	) const
	{
		// Active nearest cube should be the one that the player can stand on.
		if (const auto& near_cube = script->GetDepthNearestCube(player_pos).lock())
		{
			const auto& ntr      = near_cube->GetComponent<Components::Transform>().lock();
			const auto& cube_pos = ntr->GetWorldPosition();
			const auto& new_pos  = Vector3
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

	void FezPlayerScript::UpdateRotate(const float dt)
	{
		if (GetOwner().expired())
		{
			return;
		}

		const auto& owner = GetOwner().lock();
		const auto& tr    = owner->GetComponent<Components::Transform>().lock();
		const auto& rb    = owner->GetComponent<Components::Rigidbody>().lock();
		const auto& cldr  = owner->GetComponent<Components::Collider>().lock();

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
			m_rotate_finished_    = false;
			m_rotate_consecutive_ = false;

			return;
		}

		// CW
		if (GetApplication().HasKeyChanged(Keyboard::Q))
		{
			m_rotation_count_ = (m_rotation_count_ + 3) % 4;
			rotating          = true;
		}

		// CCW
		if (GetApplication().HasKeyChanged(Keyboard::E))
		{
			m_rotation_count_ = (m_rotation_count_ + 1) % 4;
			rotating          = true;
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

			const auto& scene  = owner->GetScene().lock();
			const auto& octree = scene->GetObjectTree();
			if (!scene)
			{
				return;
			}

			Strong<CubifyScript> ladder;

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
						ladder = parent->GetScript<CubifyScript>().lock();
					}
					else
					{
						ladder = obj->GetScript<CubifyScript>().lock();
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

			CubifyScript::DispatchNormalUpdate();

			// Find the ground and ask for other cubes whether the player can stand on.
			for (const auto& nearest = octree.Nearest(m_last_spin_position_, 1.5f);
			     const auto& obj : nearest)
			{
				const auto& candidate = obj.lock();
				if (!candidate)
				{
					continue;
				}

				Strong<CubifyScript> script;

				if (const auto& parent = candidate->GetParent().lock())
				{
					script = parent->GetScript<CubifyScript>().lock();
				}
				else
				{
					script = candidate->GetScript<CubifyScript>().lock();
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
						CubifyScript::DispatchUpdateWithoutNormal();
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

	void FezPlayerScript::DoInitialJump(const StrongRigidbody& rb, const Vector3& up)
	{
		rb->AddT1Force(up * s_jump_initial_speed);
		m_state_ = CHAR_STATE_JUMP;
		// Change the layer to none so that the player can jump through the cube.
		IgnoreCollision();
	}

	void FezPlayerScript::UpdateInitialJump()
	{
		if (GetOwner().expired())
		{
			return;
		}

		const auto& owner = GetOwner().lock();
		const auto& tr    = owner->GetComponent<Components::Transform>().lock();
		const auto& rb    = owner->GetComponent<Components::Rigidbody>().lock();

		if (!tr || !rb)
		{
			return;
		}
		if (!owner->GetActive() || !tr->GetActive() || !rb->GetActive())
		{
			return;
		}

		const auto& key_state = GetApplication().GetCurrentKeyState();
		const auto& up        = tr->Up();
		const auto& scene     = owner->GetScene().lock();

		// Discrete key check for initial jump. Continuous key check for jump will be done in jump state.
		if (GetApplication().HasKeyChanged(Keyboard::Space))
		{
			DoInitialJump(rb, up);
		}
	}

	void FezPlayerScript::UpdateJump()
	{
		if (GetOwner().expired())
		{
			return;
		}

		const auto& owner = GetOwner().lock();
		const auto& tr    = owner->GetComponent<Components::Transform>().lock();
		const auto& rb    = owner->GetComponent<Components::Rigidbody>().lock();

		if (!tr || !rb)
		{
			return;
		}
		if (!owner->GetActive() || !tr->GetActive() || !rb->GetActive())
		{
			return;
		}

		const auto& up    = tr->Up();
		const auto& scene = owner->GetScene().lock();

		if (GetApplication().IsKeyPressed(Keyboard::W) || GetApplication().IsKeyPressed(Keyboard::Space))
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

	void FezPlayerScript::UpdateFall()
	{
		if (GetOwner().expired())
		{
			return;
		}

		const auto& owner = GetOwner().lock();
		const auto& tr    = owner->GetComponent<Components::Transform>().lock();
		const auto& rb    = owner->GetComponent<Components::Rigidbody>().lock();

		if (!tr || !rb)
		{
			return;
		}
		if (!owner->GetActive() || !tr->GetActive() || !rb->GetActive())
		{
			return;
		}

		if (rb->GetGrounded())
		{
			m_state_ = CHAR_STATE_IDLE;
		}
	}

	void FezPlayerScript::UpdateGrounded()
	{
		if (GetOwner().expired())
		{
			return;
		}

		const auto& owner = GetOwner().lock();
		const auto& scene = owner->GetScene().lock();
		const auto& tr    = owner->GetComponent<Components::Transform>().lock();
		const auto& rb    = owner->GetComponent<Components::Rigidbody>().lock();
		const auto& cldr  = owner->GetComponent<Components::Collider>().lock();

		if (!tr || !rb || !scene || !cldr)
		{
			return;
		}
		if (!owner->GetActive() || !tr->GetActive() || !rb->GetActive() || !cldr->GetActive())
		{
			return;
		}

		const auto& up     = tr->Up();
		const auto& center = tr->GetWorldPosition();
		const auto& down   = -up;
		const auto& pos    = tr->GetLocalPosition();
		bool        hit    = false;

		const auto& octree = scene->GetObjectTree();
		octree.Iterate
				(
				 pos, [&hit, &owner, &cldr, &center](const WeakObjectBase& obj)
				 {
					 if (const auto& locked = obj.lock())
					 {
						 if (locked == owner)
						 {
							 return false;
						 }
						 if (!GetCollisionDetector().IsCollisionLayer(owner->GetLayer(), locked->GetLayer()))
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
			rb->SetGrounded(true);
		}
	}

	bool FezPlayerScript::doInitialClimb(
		const StrongTransform& tr, const Vector3& pos, const WeakObjectBase& obj, bool& continues
	)
	{
		continues = false;

		if (const auto& locked = obj.lock())
		{
			const auto&                     parent = locked->GetParent().lock();
			boost::shared_ptr<CubifyScript> script;

			if (!parent)
			{
				script = locked->GetScript<CubifyScript>().lock();
			}
			else
			{
				script = parent->GetScript<CubifyScript>().lock();
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
					const auto& ntr      = nearest_cube->GetComponent<Components::Transform>().lock();
					const auto& cube_pos = ntr->GetWorldPosition();

					const auto& new_pos = Vector3
					{
						m_rotation_count_ == 1 || m_rotation_count_ == 3 ? cube_pos.x : pos.x,
						pos.y,
						m_rotation_count_ == 0 || m_rotation_count_ == 2 ? cube_pos.z : pos.z
					};

					tr->SetWorldPosition(new_pos);
					IgnoreGravity();

					m_state_      = CHAR_STATE_CLIMB;
					m_b_climbing_ = true;
					return true;
				}
			}
		}
		return false;
	}

	void FezPlayerScript::UpdateInitialClimb()
	{
		if (GetOwner().expired())
		{
			return;
		}

		const auto& owner = GetOwner().lock();
		const auto& tr    = owner->GetComponent<Components::Transform>().lock();
		const auto& rb    = owner->GetComponent<Components::Rigidbody>().lock();

		if (!tr || !rb)
		{
			return;
		}
		if (!owner->GetActive() || !tr->GetActive() || !rb->GetActive())
		{
			return;
		}

		const auto& pos = tr->GetWorldPosition();

		const auto& scene  = owner->GetScene().lock();
		const auto& octree = scene->GetObjectTree();
		const auto& cldr   = owner->GetComponent<Components::Collider>().lock();

		if (GetApplication().HasKeyChanged(Keyboard::W))
		{
			const auto& nearest = octree.Hitscan(tr->GetWorldPosition(), tr->Forward(), 1);

			for (const auto& obj : nearest)
			{
				bool continues;
				if (doInitialClimb(tr, pos, obj, continues))
				{
					break;
				}
				if (continues) { }
			}

			const auto& ids = cldr->GetCollidedObjects();

			for (const auto& id : ids)
			{
				const auto obj = scene->FindGameObject(id).lock();
				if (!obj)
				{
					continue;
				}
				bool continues = false;

				if (doInitialClimb(tr, pos, obj, continues))
				{
					break;
				}
				if (continues) { }
			}
		}
	}

	void FezPlayerScript::UpdateClimb()
	{
		if (GetOwner().expired())
		{
			return;
		}

		const auto& owner = GetOwner().lock();
		const auto& tr    = owner->GetComponent<Components::Transform>().lock();
		const auto& rb    = owner->GetComponent<Components::Rigidbody>().lock();

		if (!tr || !rb)
		{
			return;
		}
		if (!owner->GetActive() || !tr->GetActive() || !rb->GetActive())
		{
			return;
		}

		const auto& up    = tr->Up();
		const auto& down  = -up;
		const auto& right = tr->Right();
		const auto& left  = -right;

		const auto& key_state = GetApplication().GetCurrentKeyState();
		const auto& pos       = tr->GetLocalPosition();
		const auto& scene     = owner->GetScene().lock();
		const auto& octree    = scene->GetObjectTree();

		const auto& nearest = octree.Nearest(pos, (tr->GetLocalScale().y * 0.5f) - g_epsilon);

		bool pressed = false;

		if (std::ranges::find_if
		    (
		     nearest, [](const WeakObjectBase& obj)
		     {
			     if (const auto& locked = obj.lock())
			     {
				     boost::shared_ptr<CubifyScript> script;

				     if (const auto& parent = locked->GetParent().lock())
				     {
					     script = parent->GetScript<CubifyScript>().lock();
				     }
				     else
				     {
					     script = locked->GetScript<CubifyScript>().lock();
				     }

				     if (script)
				     {
					     if (script->GetCubeType() == CUBE_TYPE_LADDER)
					     {
						     return true;
					     }
				     }
			     }

			     return false;
		     }
		    ) == nearest.end())
		{
			ApplyGravity();
			m_state_      = CHAR_STATE_FALL;
			m_b_climbing_ = false;
			return;
		}

		if (key_state.W)
		{
			pressed = true;
			rb->AddT1Force(up * 1.f);
		}
		if (key_state.S)
		{
			pressed = true;
			rb->AddT1Force(down * 1.f);
		}
		if (key_state.D)
		{
			pressed = true;
			rb->AddT1Force(right * 1.f);
		}
		if (key_state.A)
		{
			pressed = true;
			rb->AddT1Force(left * 1.f);
		}

		// If the player is previously climbing, revert back to the climb state.
		if (m_state_ == CHAR_STATE_POST_ROTATE && pressed && m_rotate_finished_)
		{
			m_state_ = CHAR_STATE_CLIMB;
		}
	}

	void FezPlayerScript::doDownVault(const StrongTransform& tr)
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

	void FezPlayerScript::UpdateInitialVault()
	{
		if (GetOwner().expired())
		{
			return;
		}

		const auto& owner = GetOwner().lock();
		const auto& tr    = owner->GetComponent<Components::Transform>().lock();
		const auto& rb    = owner->GetComponent<Components::Rigidbody>().lock();
		const auto& cldr  = owner->GetComponent<Components::Collider>().lock();

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
		if (GetApplication().IsKeyPressed(Keyboard::S) &&
		    (m_state_ == CHAR_STATE_IDLE || m_state_ == CHAR_STATE_WALK))
		{
			doDownVault(tr);
			m_b_vaulting_ = true;
			return;
		}

		// Check whether caught the cube, if so, set the player's state to vault.
		// Use the HasKeyChanged to avoid the easy vaulting by pressing the key.
		if (GetApplication().HasKeyChanged(Keyboard::W) &&
		    (m_state_ == CHAR_STATE_JUMP || m_state_ == CHAR_STATE_FALL))
		{
			for (const auto& id : cldr->GetCollidedObjects())
			{
				const auto& candidate = scene->FindGameObject(id).lock();
				if (!candidate)
				{
					continue;
				}

				const auto& script = candidate->GetScript<CubifyScript>().lock();
				if (!script)
				{
					continue;
				}

				if (const auto& nearest_cube = script->GetDepthNearestCube(tr->GetWorldPosition()).lock())
				{
					const auto& ntr        = nearest_cube->GetComponent<Components::Transform>().lock();
					const auto& cube_pos   = ntr->GetWorldPosition();
					const auto& player_pos = tr->GetWorldPosition();
					const auto& new_pos    = Vector3
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

					m_state_      = CHAR_STATE_VAULT;
					m_b_vaulting_ = true;
					return;
				}
			}
		}
	}

	void FezPlayerScript::UpdateVault()
	{
		if (GetOwner().expired())
		{
			return;
		}

		const auto& owner = GetOwner().lock();
		const auto& tr    = owner->GetComponent<Components::Transform>().lock();
		const auto& rb    = owner->GetComponent<Components::Rigidbody>().lock();
		const auto& cldr  = owner->GetComponent<Components::Collider>().lock();

		if (!tr || !rb || !cldr)
		{
			return;
		}
		if (!owner->GetActive() || !tr->GetActive() || !rb->GetActive() || !cldr->GetActive())
		{
			return;
		}

		const auto& up    = tr->Up();
		const auto& down  = -up;
		const auto& right = tr->Right();
		const auto& left  = -right;

		const auto& scene = owner->GetScene().lock();

		const auto& key_state = GetApplication().GetCurrentKeyState();
		const auto& pos       = tr->GetLocalPosition();

		bool pressed = false;

		// todo: slight overhead.
		if (rb->IsFixed())
		{
			// Revert the fixed state.
			ApplyLerp();
		}

		// Moving while in vault state.
		if (key_state.D)
		{
			pressed = true;
			rb->AddT1Force(right * 1.f);
		}
		if (key_state.A)
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
		if (GetApplication().HasKeyChanged(Keyboard::W))
		{
			const auto& size = tr->GetLocalScale();
			tr->SetLocalPosition(pos + Vector3{0.f, size.y / 2, 0.f});
			ApplyGravity();
			Fullstop();

			// Revert the layer to default.
			ApplyCollision();

			m_state_ = CHAR_STATE_IDLE;
			return;
		}

		// Vaulting down. (falling)
		if (GetApplication().HasKeyChanged(Keyboard::S))
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

				const auto& script = candidate->GetScript<CubifyScript>().lock();
				if (!script)
				{
					continue;
				}

				if (const auto& nearest_cube = script->GetDepthNearestCube(tr->GetWorldPosition()).lock())
				{
					const auto& ntr         = nearest_cube->GetComponent<Components::Transform>().lock();
					const auto& cube_size   = ntr->GetLocalScale();
					const auto& cube_pos    = ntr->GetWorldPosition();
					const auto& player_size = tr->GetLocalScale();
					const auto& player_pos  = tr->GetWorldPosition();
					const auto& new_pos     = Vector3
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
}
