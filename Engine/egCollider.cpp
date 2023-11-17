#include "pch.hpp"
#include "egCollider.hpp"

#include "egCollision.h"

namespace Engine::Component
{
	void Collider::SetPosition(const Vector3& position)
	{
		m_position_ = position;
		SetPosition_GENERAL_TYPE(m_boundings_.box, m_position_);
	}

	void Collider::SetRotation(const Quaternion& rotation)
	{
		m_rotation_	= rotation;
		SetRotation_GENERAL_TYPE(m_boundings_.box, m_rotation_);
	}

	void Collider::SetSize(const Vector3& size)
	{
		m_size_ = size;
		SetSize_GENERAL_TYPE(m_boundings_.box, m_size_);
	}

	void Collider::SetType(const eBoundingType type)
	{
		m_type_ = type;

		if (const auto mesh = GetOwner().lock()->GetResource<Resources::Mesh>().lock())
		{
			GenerateFromMesh(mesh);
		}

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
		const auto mesh = GetOwner().lock()->GetResource<Resources::Mesh>().lock();
		const auto other_mesh = other.GetOwner().lock()->GetResource<Resources::Mesh>().lock();

		const auto vertices = mesh->GetVertices();
		const auto other_vertices = other_mesh->GetVertices();

		auto dir = other.GetPosition() - GetPosition();
		dir.Normalize();

		Physics::GJKAlgorithm(*this, other, dir, normal, depth);
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

	void Collider::FixedUpdate(const float& dt)
	{
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
