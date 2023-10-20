#include "pch.hpp"
#include "egCollider.hpp"

namespace Engine::Component
{
	void Collider::SetPosition(const Vector3& position)
	{
		m_position_ = position;
	}

	void Collider::SetRotation(const Quaternion& rotation)
	{
		m_rotation_	= rotation;
	}

	void Collider::SetSize(const Vector3& size)
	{
		m_size_ = size;
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
		else if (m_type_ == BOUNDING_TYPE_FRUSTUM)
		{
			assert(false);
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
		else if (m_type_ == BOUNDING_TYPE_FRUSTUM)
		{
			assert(false);
		}

		return false;
	}

	void Collider::GenerateFromMesh(const std::weak_ptr<Resources::Mesh>& mesh)
	{
		const auto mesh_obj = mesh.lock();

		std::vector<Vector3> vertices;
		std::transform(std::execution::par, mesh_obj->m_vertices_.begin(), mesh_obj->m_vertices_.end(), vertices.begin(), [](const auto& vertex)
		{
			return vertex.position;
		});

		if (m_type_ == BOUNDING_TYPE_BOX)
		{
			BoundingOrientedBox::CreateFromPoints(m_boundings_.box, mesh_obj->m_vertices_.size(), vertices.data(), sizeof(Vector3));
		}
		else if (m_type_ == BOUNDING_TYPE_SPHERE)
		{
			BoundingSphere::CreateFromPoints(m_boundings_.sphere, mesh_obj->m_vertices_.size(), vertices.data(), sizeof(Vector3));
		}
		else if (m_type_ == BOUNDING_TYPE_FRUSTUM)
		{
			assert(false);
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
		else if (m_type_ == BOUNDING_TYPE_FRUSTUM)
		{
			assert(false);
		}
	}
}
