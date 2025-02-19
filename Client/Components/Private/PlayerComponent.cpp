#include "Components/Public/PlayerComponent.h"
#include "PlayerComponent.generated.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include "Components/Public/PlayerHitboxComponent.h"
#include "Components/Public/WeaponComponent.h"

#include "ObjectBase/Public/ObjectBase.h"
#include "Objects/Camera/Public/Camera.h"
#include "InputManager.h"
#include "SceneManager/Public/SceneManager.h"
#include "CollisionDetector.h"
#include "InputInterface.h"

#include "Components/Transform/Public/Transform.h"
#include "Components/Collider/Public/Collider.h"
#include "Components/Rigidbody/Public/Rigidbody.h"
#include "Animator.h"
#include "ModelRenderer.h"
#include "ShapeRenderComponent.h"
#include "Shape.h"

void PlayerComponent::Initialize()
{
	Component::Initialize();
	setState(CHAR_STATE_IDLE);

	if (const auto& owner = GetOwner().lock())
	{
		const auto& tr = owner->AddComponent<Engine::Components::Transform>().lock();
		const auto& mr = owner->AddComponent<Engine::Components::ModelRenderer>().lock();
		const auto& rb = owner->AddComponent<Engine::Components::Rigidbody>().lock();
		const auto& atr = owner->AddComponent<Engine::Components::Animator>().lock();
		const auto& cldr = owner->AddComponent<Engine::Components::Collider>().lock();

		const auto& shape = Engine::Resources::Shape::Get("CharacterShape").lock();
		mr->SetShape(shape);

		Engine::ShapeExtension::BindShapeToCollider(shape, cldr);
		cldr->SetType(Engine::BOUNDING_TYPE_BOX);
		cldr->SetMass(1.0f);

		rb->SetFrictionCoefficient(0.1f);
		rb->SetGravityOverride(true);
		rb->SetNoAngular(true);

		atr->SetAnimation(0);
	}

	// todo: determine local player
	//MoveCameraToChild(true);
}

void PlayerComponent::PreUpdate(const float dt)
{
	m_prev_state_ = m_state_;
}

void PlayerComponent::PostUpdate(const float dt)
{
}

void PlayerComponent::Update(const float dt)
{
	if (Engine::Managers::SceneManager::GetInstance().IsPlaying())
	{
		const auto rb = GetOwner().lock()->GetComponent<Engine::Components::Rigidbody>().lock();

		if (!rb)
		{
			return;
		}

		if (const auto head = getHead().lock();
			head && !m_top_view_)
		{
			const auto head_tr = head->GetComponent<Engine::Components::Transform>().lock();
			const auto mouse_y = Engine::Managers::InputManager::GetInstance().GetMouseYRotation();
			head_tr->SetLocalRotation(mouse_y);
		}

		const auto body_tr = GetOwner().lock()->GetComponent<Engine::Components::Transform>().lock();
		const auto mouse_x = Engine::Managers::InputManager::GetInstance().GetMouseYRotation();
		body_tr->SetLocalRotation(mouse_x);

		checkJump(rb);
		checkMove(rb);
		checkAttack(dt);
	}

#if WITH_DEBUG
	switch (getState())
	{
	case CHAR_STATE_IDLE:
		if (hasStateChanged())
		{
			Engine::Managers::Debugger::GetInstance().Log("Idle", {1.0f, 0.f, 0.f, 1.f});
		}
		break;
	case CHAR_STATE_WALK:
		if (hasStateChanged())
		{
			Engine::Managers::Debugger::GetInstance().Log("Walk", { 1.0f, 0.f, 0.f, 1.f });
		}
		break;
	case CHAR_STATE_RUN:
		break;
	case CHAR_STATE_JUMP:
		if (hasStateChanged())
		{
			Engine::Managers::Debugger::GetInstance().Log("Jump", { 1.0f, 0.f, 0.f, 1.f });
		}
		break;
	case CHAR_STATE_ATTACK:
		if (hasStateChanged())
		{
			Engine::Managers::Debugger::GetInstance().Log("Attack", { 1.0f, 0.f, 0.f, 1.f });
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
#endif
}

void PlayerComponent::FixedUpdate(const float dt)
{
	checkGround();
}

void PlayerComponent::OnSerialized()
{
}

void PlayerComponent::OnDeserialized()
{
}

void PlayerComponent::SetActive(bool active)
{
	Component::SetActive(active);
	moveCameraToChild(active);
}

UINT PlayerComponent::GetHealth() const
{
	return 0;
}

void PlayerComponent::Hit(float damage)
{
}

PlayerComponent::PlayerComponent(const Engine::Weak<Engine::Abstracts::ObjectBase>& owner)
	: Component(owner)
{
}

Engine::Weak<Engine::Abstracts::ObjectBase> PlayerComponent::getHead() const
{
	if (const auto& owner = GetOwner().lock())
	{
		if (const auto& component = owner->GetComponent<PlayerHitboxComponent>().lock())
		{
			return component->GetHead();
		}
	}

	return {};
}

float PlayerComponent::getFireRate() const
{
	if (const auto owner = GetOwner().lock())
	{
		if (const auto component = owner->GetComponent<WeaponComponent>().lock())
		{
			return component->GetFireRate();
		}
	}

	return FLT_MAX;
}

void PlayerComponent::hitScan(float damage, float range) const
{
}

eCharacterState PlayerComponent::getState() const
{
	return m_state_;
}

void PlayerComponent::setState(eCharacterState state)
{
	m_state_ = state;
}

bool PlayerComponent::hasStateChanged() const
{
	return m_state_ != m_prev_state_;
}

void PlayerComponent::moveCameraToChild(bool active)
{
	if (const auto head = getHead().lock();
		const auto scene = GetOwner().lock()->GetScene().lock())
	{
		const auto cam = scene->GetMainCamera().lock();

		head->AddChild(cam);
		m_cam_id_ = cam->GetLocalID();
	}
}

void PlayerComponent::setHeadView(bool head_view)
{
	if (m_cam_id_ == Engine::g_invalid_id)
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

	const auto cam = cam_obj->GetSharedPtr<Engine::Objects::Camera>();
	const auto cam_tr = cam_obj->GetComponent<Engine::Components::Transform>().lock();

	if (m_top_view_)
	{
		constexpr float pidiv2 = M_PI / 2;

		cam->SetOrthogonal(false);
		cam_tr->SetLocalPosition({ 0.f, 10.f, 0.f });
		cam_tr->SetLocalRotation(Quaternion::CreateFromAxisAngle(Vector3::Right, pidiv2));
	}
	else
	{
		cam->SetOrthogonal(false);
		cam_tr->SetLocalPosition(Vector3::Zero);
		cam_tr->SetLocalRotation(Quaternion::Identity);
	}

	m_top_view_ = head_view;
}

void PlayerComponent::checkJump(const Engine::Strong<Engine::Components::Rigidbody>& rb)
{
	rb->SetGravityOverride(!m_b_grounded_);
}

void PlayerComponent::checkMove(const Engine::Strong<Engine::Components::Rigidbody>& rb)
{
	float      speed = 10.0f;
	const auto scene = GetOwner().lock()->GetScene().lock();

	auto    forward = GetOwner().lock()->GetComponent<Engine::Components::Transform>().lock()->Forward();
	Vector3 ortho;
	forward.Cross(Vector3::Up, ortho);

	forward *= {1.f, 0.f, 1.f};
	ortho *= {1.f, 0.f, 1.f};

	forward *= speed;
	ortho *= speed;

	bool       pressed = false;
	const auto atr = GetOwner().lock()->GetComponent<Engine::Components::Animator>().lock();

	constexpr UINT forward_anim = 20;
	constexpr UINT backward_anim = 19;
	constexpr UINT left_anim = 22;
	constexpr UINT right_anim = 21;
	constexpr UINT idle_anim = 0;

	if (Engine::Managers::InputManager::GetInstance().IsKeyPressed(Engine::Keyboard::Keys::W))
	{
		atr->SetAnimation(forward_anim);
		rb->AddT1Force(forward);
		pressed = true;
	}

	if (Engine::Managers::InputManager::GetInstance().IsKeyPressed(Engine::Keyboard::Keys::A))
	{
		atr->SetAnimation(left_anim);
		rb->AddT1Force(ortho);
		pressed = true;
	}

	if (Engine::Managers::InputManager::GetInstance().IsKeyPressed(Engine::Keyboard::Keys::S))
	{
		atr->SetAnimation(backward_anim);
		rb->AddT1Force(-forward);
		pressed = true;
	}

	if (Engine::Managers::InputManager::GetInstance().IsKeyPressed(Engine::Keyboard::Keys::D))
	{
		atr->SetAnimation(right_anim);
		rb->AddT1Force(-ortho);
		pressed = true;
	}

	if (!pressed)
	{
		setState(CHAR_STATE_IDLE);
		atr->SetAnimation(idle_anim);
	}
	else
	{
		setState(CHAR_STATE_WALK);
	}
}

void PlayerComponent::checkAttack(const float dt)
{
	if (Engine::Managers::InputManager::GetInstance().IsKeyDown(Engine::eMouseButtonEnum::MOUSE_LEFT))
	{
		setState(CHAR_STATE_ATTACK);

		const auto& fire_rate = getFireRate();

		if (m_fire_interval_ < fire_rate)
		{
			m_fire_interval_ += dt;
			return;
		}

		m_fire_interval_ = 0.f;
		const auto tr =
			GetOwner().lock()->GetComponent<Engine::Components::Transform>().lock();

		Ray ray;
		ray.position = tr->GetWorldPosition();
		ray.direction = tr->Forward();

		constexpr float distance = 5.f;

#if WITH_DEBUG
		Engine::Managers::Debugger::GetInstance().Draw(ray, {0.f, 1.f, 0.f, 1.f});
#endif

		hitScan(10.f, 10.f);
	}
}

void PlayerComponent::checkGround()
{
	const auto  scene = GetOwner().lock()->GetScene().lock();
	const auto& tree = scene->GetObjectTree();
	const auto  rb = GetOwner().lock()->GetComponent<Engine::Components::Rigidbody>().lock();

	std::queue<const Engine::Octree*> q;
	q.push(&tree);

	while (!q.empty())
	{
		const auto node = q.front();
		q.pop();

		const auto& value = node->Read();
		const auto& children = node->Next();

		for (const auto v : value)
		{
			const auto lcl = GetOwner().lock()->GetComponent<Engine::Components::Collider>().lock();
			const auto rcl = v.lock()->GetComponent<Engine::Components::Collider>().lock();

			if (!Engine::Managers::CollisionDetector::GetInstance().IsCollisionLayer(GetOwner().lock()->GetLayer(), v.lock()->GetLayer()))
			{
				continue;
			}
			if (!rcl || lcl == rcl)
			{
				continue;
			}

			const auto owner = rcl->GetOwner().lock();
			const auto owner_parent = owner->GetParent();

			if (owner_parent.lock() == GetOwner().lock())
			{
				continue;
			}

			if (Engine::Components::Collider::Intersects(lcl, rcl, Vector3::Down))
			{
				m_b_grounded_ = true;
				return;
			}
		}

		for (const auto& child : children)
		{
			if (child && child->Contains
			(
				GetOwner().lock()->GetComponent<Engine::Components::Transform>().lock()->GetWorldPosition()
			))
			{
				q.push(child);
			}
		}
	}
}

void PlayerComponent::onCollisionEnter(const Engine::Weak<Engine::Components::Collider>& other)
{
}

void PlayerComponent::onCollisionContinue(const Engine::Weak<Engine::Components::Collider>& other)
{
}

void PlayerComponent::onCollisionExit(const Engine::Weak<Engine::Components::Collider>& other)
{
}

Engine::eComponentUpdatePriorities PlayerComponent::GetUpdatePriority() const
{
	return Engine::eComponentUpdatePriority::COM_PRIORITY_POSITIONAL;
}
