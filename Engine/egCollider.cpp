#include "pch.hpp"
#include "egCollider.hpp"
#include "egResourceManager.hpp"
#include "egCollision.h"
#include "egCubeMesh.hpp"
#include "egD3Device.hpp"
#include "egDebugObject.hpp"
#include "egSceneManager.hpp"
#include "egSphereMesh.hpp"

namespace Engine::Component
{
	void Collider::SetPosition(const Vector3& position)
	{
		m_position_ = position;

		if (m_type_ == BOUNDING_TYPE_BOX)
		{
			SetPosition_GENERAL_TYPE(m_boundings_.box, m_position_);
		}
		else if (m_type_ == BOUNDING_TYPE_SPHERE)
		{
			SetPosition_GENERAL_TYPE(m_boundings_.sphere, m_position_);
		}

		UpdateBoundings();
	}

	void Collider::SetRotation(const Quaternion& rotation)
	{
		m_rotation_	= rotation;

		if (m_type_ == BOUNDING_TYPE_BOX)
		{
			SetRotation_GENERAL_TYPE(m_boundings_.box, m_rotation_);
		}
		else if (m_type_ == BOUNDING_TYPE_SPHERE)
		{
			SetRotation_GENERAL_TYPE(m_boundings_.sphere, m_rotation_);
		}

		UpdateBoundings();
		UpdateInertiaTensor();
	}

	void Collider::Initialize()
	{
		if (const auto mesh = GetOwner().lock()->GetResource<Resources::Mesh>(L"").lock())
		{
			GenerateFromMesh(mesh);
		}
		else
		{
			throw std::exception("Mesh is not loaded");
		}

		if (m_type_ == BOUNDING_TYPE_BOX)
		{
			GenerateInertiaCube();
		}
		else if (m_type_ == BOUNDING_TYPE_SPHERE)
		{
			GenerateInertiaSphere();
		}

#ifdef _DEBUG
		GenerateDebugMesh();
#endif

		UpdateFromTransform();
		UpdateInertiaTensor();
	}

	void Collider::SetSize(const Vector3& size)
	{
		m_size_ = size;

		if (m_type_ == BOUNDING_TYPE_BOX)
		{
			SetSize_GENERAL_TYPE(m_boundings_.box, m_size_);
		}
		else if (m_type_ == BOUNDING_TYPE_SPHERE)
		{
			SetSize_GENERAL_TYPE(m_boundings_.sphere, m_size_);
		}

		UpdateBoundings();
	}

	void Collider::SetType(const eBoundingType type)
	{
		m_type_ = type;

		if (const auto mesh = GetOwner().lock()->GetResource<Resources::Mesh>(L"").lock())
		{
			GenerateFromMesh(mesh);
		}

#ifdef _DEBUG
		GenerateDebugMesh();
#endif

		UpdateBoundings();
	}

	bool Collider::Intersects(Collider& other) const
	{
		if (m_type_ == BOUNDING_TYPE_BOX)
		{
			return other.Intersects_GENERAL_TYPE(m_boundings_.box);
		}
		else if (m_type_ == BOUNDING_TYPE_SPHERE)
		{
			return other.Intersects_GENERAL_TYPE(m_boundings_.sphere);
		}

		return false;
	}

	bool Collider::Intersects(const Ray& ray, float distance, float& intersection) const
	{
		if (m_type_ == BOUNDING_TYPE_BOX)
		{
			const Vector3 Extents = m_boundings_.box.Extents;
			const auto test = Engine::Physics::Ray::TestRayOBBIntersection(ray.position, ray.direction, -Extents, Extents, m_world_matrix_, intersection);

			return test && intersection <= distance;
		}
		else if (m_type_ == BOUNDING_TYPE_SPHERE)
		{
			const auto test = Engine::Physics::Ray::TestRaySphereIntersection(ray, m_boundings_.sphere.Center, m_boundings_.sphere.Radius, intersection);

			return test && intersection <= distance;
		}

		return false;
	}

	bool Collider::Contains(Collider& other) const
	{
		if (m_type_ == BOUNDING_TYPE_BOX)
		{
			return other.Contains_GENERAL_TYPE(m_boundings_.box);
		}
		else if (m_type_ == BOUNDING_TYPE_SPHERE)
		{
			return other.Contains_GENERAL_TYPE(m_boundings_.sphere);
		}

		return false;
	}

	void Collider::GetPenetration(const Collider& other, Vector3& normal, float& depth) const
	{
		const auto mesh = GetOwner().lock()->GetResource<Resources::Mesh>(L"").lock();
		const auto other_mesh = other.GetOwner().lock()->GetResource<Resources::Mesh>(L"").lock();

		const auto vertices = mesh->GetVertices();
		const auto other_vertices = other_mesh->GetVertices();

		auto dir = other.GetPosition() - GetPosition();
		dir.Normalize();

		Physics::GJK::GJKAlgorithm(*this, other, dir, normal, depth);
	}

	void Collider::GenerateFromMesh(const std::weak_ptr<Resources::Mesh>& mesh)
	{
		const auto mesh_obj = mesh.lock();

		std::vector<Vector3> serialized_vertices;

		std::ranges::for_each(mesh_obj->m_vertices_, [&](const Resources::Shape& shape)
		{
			for (int i = 0; i < shape.size(); ++i)
			{
				serialized_vertices.emplace_back(shape[i].position);
			}
		});

		if (m_type_ == BOUNDING_TYPE_BOX)
		{
			BoundingOrientedBox::CreateFromPoints(m_boundings_.box, mesh_obj->m_vertices_.size(), serialized_vertices.data(), sizeof(Vector3));
		}
		else if (m_type_ == BOUNDING_TYPE_SPHERE)
		{
			BoundingSphere::CreateFromPoints(m_boundings_.sphere, mesh_obj->m_vertices_.size(), serialized_vertices.data(), sizeof(Vector3));
		}
	}

#ifdef _DEBUG
	void Collider::GenerateDebugMesh()
	{
		if (m_debug_mesh_)
		{
			m_debug_mesh_.reset();
		}

		m_debug_mesh_ = Instantiate<Object::DebugObject>();

		if (m_type_ == BOUNDING_TYPE_BOX)
		{
			GetResourceManager().AddResource(L"CubeMesh", std::make_shared<Mesh::CubeMesh>());
			m_debug_mesh_->AddResource(
				GetResourceManager().GetResource<Mesh::CubeMesh>(L"CubeMesh"));
		}
		else if (m_type_ == BOUNDING_TYPE_SPHERE)
		{
			GetResourceManager().AddResource(L"SphereMesh", std::make_shared<Mesh::SphereMesh>());
			m_debug_mesh_->AddResource(
				GetResourceManager().GetResource<Mesh::SphereMesh>(L"SphereMesh"));
		}
	}
#endif

	void Collider::FixedUpdate(const float& dt)
	{
	}

	void Collider::Render(const float dt)
	{
#ifdef _DEBUG
		if (m_debug_mesh_)
		{
			if (m_collided_objects_.size() > 0)
			{
				m_debug_mesh_->SetCollided(true);
			}
			else
			{
				m_debug_mesh_->SetCollided(false);
			}

			m_debug_mesh_->Render(dt);
		}
#endif
	}

	void Collider::UpdateFromTransform()
	{
		if (const auto tr = GetOwner().lock()->GetComponent<Transform>().lock(); m_bDirtyByTransform)
		{
			m_position_ = tr->GetPosition();
			m_rotation_ = tr->GetRotation();
			m_size_ = tr->GetScale();

			UpdateBoundings();
		}
	}

	void Collider::UpdateBoundings()
	{
		if (m_type_ == BOUNDING_TYPE_BOX)
		{
			SetPosition_GENERAL_TYPE(m_boundings_.box, m_position_);
			SetSize_GENERAL_TYPE(m_boundings_.box, m_size_);
			SetRotation_GENERAL_TYPE(m_boundings_.box, m_rotation_);
		}
		else if (m_type_ == BOUNDING_TYPE_SPHERE)
		{
			SetPosition_GENERAL_TYPE(m_boundings_.sphere, m_position_);
			SetSize_GENERAL_TYPE(m_boundings_.sphere, m_size_);
			m_rotation_ = Quaternion::Identity;
		}

		m_world_matrix_ = Matrix::CreateWorld(Vector3::Zero, Vector3::Forward, Vector3::Up);
		m_world_matrix_ *= Matrix::CreateScale(m_size_);
		m_world_matrix_ *= Matrix::CreateFromQuaternion(m_rotation_);
		m_world_matrix_ *= Matrix::CreateTranslation(m_position_);
	}

	void Collider::UpdateInertiaTensor()
	{
		const Quaternion rotation = m_rotation_;
		Quaternion conjugate;

		rotation.Conjugate(conjugate);
		const Matrix invOrientation = Matrix::CreateFromQuaternion(conjugate);
		const Matrix orientation = Matrix::CreateFromQuaternion(rotation);

		const Matrix matrix = orientation * XMMatrixScaling(GetSize().x, GetSize().y, GetSize().z) * invOrientation;

		XMStoreFloat3x3(&m_inertia_tensor_, matrix);
	}
}
