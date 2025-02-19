#pragma once
#include "Client.h"
#include "Component/Public/Component.h"
#include "Verlet.hpp"

#include "FezPlayerComponent.generated.h"

class CubifyComponent;

ECLASS(component=client, serialize)
class ENGINE_CLIENT_API FezPlayerComponent : public Engine::Abstracts::Component
{
	// Vector3::Up is non-const static
	constexpr static Vector3 s_up = { 0, 1, 0 };
	constexpr static float   s_rotation_speed = 1.f;

	// Clockwise movement
	// Since forward is facing to the screen, rotation is inverted.
	static const Quaternion s_cw_rotations[4];

	GENERATE_BODY

	explicit FezPlayerComponent(const Engine::Weak<Engine::Abstracts::ObjectBase>& owner);

	void Initialize() override;
    void BeginPlay( const float dt ) override;
	void PreUpdate(const float dt) override;
	void Update(const float dt) override;
	void PostUpdate(const float dt) override;
	void FixedUpdate(const float dt) override;

	void SetRotateAllowed(const bool allowed)
	{
		m_rotate_allowed_ = allowed;
	}

	eCharacterState GetState() const
	{
		return m_state_;
	}

	eCharacterState GetPrevState() const
	{
		return m_prev_state_;
	}

	Vector3 GetForward() const;

	UINT GetRotationOffset() const
	{
		return m_rotation_count_;
	}

	bool IsVisible(const Engine::Weak<Engine::Components::Transform>& otr) const;

	void OnSerialized() override;
	void OnDeserialized() override;
	Engine::eComponentUpdatePriorities GetUpdatePriority() const override;

#if WITH_EDITOR
	void OnUIUpdate(Engine::UIContext* const parent, const float dt) override;
#endif

protected:
	void onCollisionEnter(const Engine::Weak<Engine::Components::Collider>& other);
	void onCollisionContinue(const Engine::Weak<Engine::Components::Collider>& other);
	void onCollisionExit(const Engine::Weak<Engine::Components::Collider>& other);

private:
	FezPlayerComponent();
	constexpr static float s_jump_speed = -Engine::g_gravity_vec.y;
	constexpr static float s_jump_initial_speed = s_jump_speed * 3.f;
	constexpr static float s_jump_apex = 10.f;

	// Utilities
	void IgnoreCollision() const;
	void ApplyCollision() const;
	void IgnoreGravity() const;
	void ApplyGravity() const;
	void IgnoreLerp() const;
	void ApplyLerp() const;
	void Fullstop() const;
	void MoveCameraToChild() const;

	// State changes
	void UpdateMove();
	void UpdateRotate(float dt);
	void UpdateGrounded();

	void UpdateInitialJump();
	void UpdateJump();
	void UpdateFall();

	void UpdateInitialClimb();
	void UpdateClimb();

	void UpdateInitialVault();
	void UpdateVault();

	// Subroutine for state changes
	void DoInitialJump(const Engine::Strong<Engine::Components::Rigidbody>& rb, const Vector3& up);

	bool doInitialClimb(
		const Engine::Strong<Engine::Components::Transform>& tr, const Vector3& pos, const Engine::Weak<Engine::Abstracts::ObjectBase>& obj, bool& continues
	);

	bool movePlayerToNearestCube(
		const Engine::Strong<Engine::Components::Transform>& tr,
		const Engine::Strong<CubifyComponent>& script,
		const Vector3& player_pos
	) const;

	void doDownVault(const Engine::Strong<Engine::Components::Transform>& tr);

	EPROPERTY()
	eCharacterState m_state_;
	EPROPERTY()
	eCharacterState m_prev_state_;
	EPROPERTY()
	bool m_b_grounded_;

	// Rotation variables
	// Count of 90 degree rotations
	EPROPERTY()
	UINT m_rotation_count_;
	// Animation time for rotation
	EPROPERTY()
	float m_accumulated_dt_;

	// Last position of player when rotating
	EPROPERTY()
	Vector3 m_last_spin_position_;

	// Flag for whether player has red hat.
	EPROPERTY()
	bool m_rotate_allowed_;
	// Flag for whether player has finished rotating
	EPROPERTY()
	bool m_rotate_finished_;
	// Flag for whether player has consecutive rotations
	EPROPERTY()
	bool m_rotate_consecutive_;

	// Climb variables
	EPROPERTY()
	bool m_b_climbing_;
	EPROPERTY()
	bool m_b_vaulting_;
};