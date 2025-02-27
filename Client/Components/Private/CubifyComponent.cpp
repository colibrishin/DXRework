#include "Components/Public/CubifyComponent.h"
#include "CubifyComponent.generated.h"

#include "Scene.h"
#include "ObjectBase.h"
#include "Transform.h"
#include "SceneManager.h"
#include "Camera.h"
#include "Collider.h"
#include "Object.h"
#include "Components/Public/FezPlayerComponent.h"

using namespace Engine;

void CubifyComponent::Initialize()
{
    Component::Initialize();

    if (m_cube_type_ == CUBE_TYPE_NORMAL)
    {
        UpdateCubes(true);
    }
    else
    {
        UpdateCubes(false);
    }
}

void CubifyComponent::PreUpdate(const float dt)
{
}

void CubifyComponent::Update(const float dt)
{
}

void CubifyComponent::UpdateCubes(bool normal)
{
	Engine::Strong<Engine::Abstracts::ObjectBase> owner = GetOwner().lock();
	
    if (!owner)
    {
        return;
    }

    Engine::Strong<Engine::Scene> scene = owner->GetScene().lock();

    if (!scene) 
    {
        return;
    }
    
    Engine::Strong<Engine::Abstracts::ObjectBase> player = scene->GetMainActor().lock();

    if (!player)
    {
        return;
    }

	const auto& player_comp = player->GetComponent<FezPlayerComponent>().lock();
	if (!player_comp)
	{
		return;
	}

	updateCubesImpl(normal);
}

void CubifyComponent::PostUpdate(const float dt)
{
}

void CubifyComponent::FixedUpdate(const float dt)
{
}

void CubifyComponent::SetCubeDimension(const Vector3& dimension)
{
    if (dimension == Vector3::Zero)
    {
        return;
    }
    if (dimension.x <= 0 || dimension.y <= 0 || dimension.z <= 0)
    {
        return;
    }
    m_cube_dimension_ = dimension;
}

void CubifyComponent::SetCubeType(eCubeType type)
{
    m_cube_type_ = type;
}

Engine::Weak<Engine::Abstracts::ObjectBase> CubifyComponent::GetDepthNearestCube(const Vector3& pos) const
{
	const auto& owner = GetOwner().lock();

	if (!owner)
	{
		return {};
	}

	Engine::Weak<Engine::Abstracts::ObjectBase> nearest_cube;
	float          nearest_distance = std::numeric_limits<float>::max();

	for (const auto& id : m_cube_ids_)
	{
		if (const auto& cube = owner->GetChild(id).lock())
		{
			const auto& tr = cube->GetComponent<Engine::Components::Transform>().lock();

			if (!tr)
			{
				continue;
			}
			// If cube is not active, then cube is not in the view.
			if (!cube->GetActive())
			{
				continue;
			}

			// Blocking player teleporting to upper side.
			if (tr->GetWorldPosition().y > pos.y)
			{
				continue;
			}

			const auto& distance = Vector3::DistanceSquared(tr->GetWorldPosition(), pos);

			if (distance < nearest_distance)
			{
				nearest_distance = distance;
				nearest_cube = cube;
			}
		}
	}

	return nearest_cube;
}

eCubeType CubifyComponent::GetCubeType() const
{
	return m_cube_type_;
}

void CubifyComponent::DispatchNormalUpdate()
{
	Engine::Strong<Engine::Scene> scene;

	if (const Engine::Strong<Engine::Scene>& scene = Managers::SceneManager::GetInstance().GetActiveScene().lock())
	{
		const auto& others = scene->GetCachedComponents<CubifyComponent>();

		for (const auto& comp : others)
		{
			if (const Strong<CubifyComponent>& component = Cast<CubifyComponent>(comp))
			{
				if (component->GetCubeType() == CUBE_TYPE_NORMAL)
				{
					component->UpdateCubes(true);
				}
			}
		}
	}
}

void CubifyComponent::DispatchUpdateWithoutNormal()
{
	Engine::Strong<Engine::Scene> scene;

	if (const Engine::Strong<Engine::Scene>& scene = Managers::SceneManager::GetInstance().GetActiveScene().lock())
	{
		const auto& others = scene->GetCachedComponents<CubifyComponent>();

		for (const auto& comp : others)
		{
			if (const Strong<CubifyComponent>& component = Cast<CubifyComponent>(comp))
			{
				if (component->GetCubeType() != CUBE_TYPE_NORMAL)
				{
					component->UpdateCubes(false);
				}
			}
		}
	}
}

void CubifyComponent::onCollisionEnter(const Engine::Weak<Engine::Components::Collider>& other)
{
}

void CubifyComponent::onCollisionContinue(const Engine::Weak<Engine::Components::Collider>& other)
{
}

void CubifyComponent::onCollisionExit(const Engine::Weak<Engine::Components::Collider>& other)
{
}

CubifyComponent::CubifyComponent(const Engine::Weak<Engine::Abstracts::ObjectBase>& owner) :
    Component(owner),
    m_cube_dimension_(Vector3::One),
    m_cube_type_(CUBE_TYPE_NORMAL),
    m_z_length_(0),
    m_y_length_(0),
    m_x_length_(0) 
{
}

CubifyComponent::CubifyComponent() :
	Component({}),
	m_cube_dimension_(Vector3::One),
	m_cube_type_(CUBE_TYPE_NORMAL),
	m_z_length_(0),
	m_y_length_(0),
	m_x_length_(0)
{
}

void CubifyComponent::updateCubesImpl(bool normal)
{
	MathExtension::ZeroToEpsilon(m_cube_dimension_);

	// Cube statics
	const auto x_step = m_cube_dimension_.x;
	const auto y_step = m_cube_dimension_.y;
	const auto z_step = m_cube_dimension_.z;

	const auto x_step_half = x_step * 0.5f;
	const auto y_step_half = y_step * 0.5f;
	const auto z_step_half = z_step * 0.5f;

	const float move_step_by_rotation[4] =
	{
		x_step,
		z_step,
		x_step,
		z_step
	};

	const Vector3 move_offset_by_rotation[4] =
	{
		s_move_offsets[0] * x_step,
		s_move_offsets[1] * z_step,
		s_move_offsets[2] * x_step,
		s_move_offsets[3] * z_step
	};

	// Fix values of axis with player's value, to correct the cube with player's movement.
	// This values are same as the camera's forward.
	static const Vector3 fixed_axis[4] =
	{
		g_backward,
		Vector3::Left,
		g_forward,
		Vector3::Right
	};

	if (const auto& owner = GetOwner().lock())
	{
		Strong<Scene> scene = owner->GetScene().lock();
		if (!scene)
		{
			return;
		}
		
		Strong<Abstracts::ObjectBase> player = scene->GetMainActor().lock();
		if (!player)
		{
			return;
		}

		Strong<Objects::Camera> camera = scene->GetMainCamera().lock();
		if (!camera)
		{
			return;
		}

		const auto& cldr = owner->GetComponent<Components::Collider>().lock();
		const auto& tr = owner->GetComponent<Components::Transform>().lock();
		if (!cldr || !tr)
		{
			return;
		}

		const auto& scale = tr->GetLocalScale();
		const auto& half_scale = scale * 0.5f;
		const auto& cam_forward = camera->GetComponent<Components::Transform>().lock()->Forward();
		const auto& player_tr = player->GetComponent<Components::Transform>().lock();
		const auto& player_component = player->GetComponent<FezPlayerComponent>().lock();
		const auto& player_pos = player_tr->GetWorldPosition();
		const auto& obj_pos = tr->GetWorldPosition();

		const int rotation_offset = player_component->GetRotationOffset();

		for (const auto& id : m_cube_ids_)
		{
			if (const auto& cube = owner->GetChild(id).lock())
			{
				scene->RemoveGameObject(cube->GetID(), cube->GetLayer());
			}
		}

		m_cube_ids_.clear();

		if (m_cube_type_ != CUBE_TYPE_NORMAL)
		{
			if (!player_component->IsVisible(tr))
			{
				return;
			}
		}

		const float x_count = half_scale.x / x_step;
		const float y_count = half_scale.y / y_step;
		const float z_count = half_scale.z / z_step;

		m_z_length_ = static_cast<int>(z_count * 2.f);
		m_y_length_ = static_cast<int>(y_count * 2.f);
		m_x_length_ = static_cast<int>(x_count * 2.f);

		// todo: refactoring
		const Vector3 start_pos_by_rotation[4] =
		{
			{-half_scale.x + x_step_half, -half_scale.y + y_step_half, -half_scale.z + z_step_half},
			{half_scale.x - x_step_half, -half_scale.y + y_step_half, -half_scale.z + z_step_half},
			{half_scale.x - x_step_half, -half_scale.y + y_step_half, half_scale.z - z_step_half},
			{-half_scale.x + x_step_half, -half_scale.y + y_step_half, half_scale.z - z_step_half}
		};

		Vector3 start_pos = start_pos_by_rotation[rotation_offset];

		if (!normal && m_cube_type_ != CUBE_TYPE_NORMAL)
		{
			const auto& delta = player_pos - obj_pos;
			const auto& axis_offset = fixed_axis[rotation_offset] * delta.Dot(fixed_axis[rotation_offset]);
			start_pos = start_pos + axis_offset;
		}

		float y = start_pos.y;

		const int target_count = rotation_offset == 0 || rotation_offset == 3 ? m_x_length_ : m_z_length_;

		for (int i = 0; i < m_y_length_; ++i)
		{
			start_pos = start_pos_by_rotation[rotation_offset];

			if (!normal && m_cube_type_ != CUBE_TYPE_NORMAL)
			{
				const auto& delta = player_pos - obj_pos;
				const auto& proj = delta.Dot(fixed_axis[rotation_offset]);
				const auto& axis_offset = fixed_axis[rotation_offset] * proj;
				start_pos = start_pos + axis_offset;
			}

			start_pos.y = y;

			for (int j = 0; j < target_count; ++j)
			{
				const auto& cube = scene->CreateGameObject<Object>(RESERVED_LAYER_ENVIRONMENT).lock();
				cube->SetName
				(
					owner->GetName() + " Cube " + std::format
					("{}_{}_{}", start_pos.x, start_pos.y, start_pos.z)
				);
				const auto& cube_tr = cube->AddComponent<Components::Transform>().lock();

				cube->AddComponent<Components::Collider>();

				owner->AddChild(cube, true);

				cube_tr->SetLocalScale(m_cube_dimension_);
				// Note that this uses local space.
				cube_tr->SetLocalPosition(start_pos);

				m_cube_ids_.push_back(cube->GetLocalID());

				start_pos += move_offset_by_rotation[rotation_offset];
			}

			y += y_step;
		}
	}
}

void CubifyComponent::OnSerialized()
{
}

void CubifyComponent::OnDeserialized()
{
}

eComponentUpdatePriorities CubifyComponent::GetUpdatePriority() const
{
	return eComponentUpdatePriority::COM_PRIORITY_POSITIONAL;
}

#if WITH_EDITOR
void CubifyComponent::OnUIUpdate(Engine::UIContext* const parent, const float dt)
{
	if (parent)
	{
		Component::OnUIUpdate(parent, dt);
		IUIAPI& ui = s_uia.GetInterface();

		(*parent |= ui.NewLabelAndVec3(this, "CubeDimension", { "Cube Dimension", &m_cube_dimension_.x, 0.1f, 0.1f, FLT_MAX, true })).SetFunction([]()
			{
				DispatchNormalUpdate();
				DispatchUpdateWithoutNormal();
			});

		static constexpr auto cube_type_enum = CStrEnumStrings<eCubeType>();

		(*parent |= ui.NewCombobox(this, "CubeType", { "Cube Type", reinterpret_cast<int*>(&m_cube_type_), cube_type_enum.data(), cube_type_enum.size(), true })).SetFunction([]()
			{
				DispatchNormalUpdate();
				DispatchUpdateWithoutNormal();
			});
	}
}
#endif