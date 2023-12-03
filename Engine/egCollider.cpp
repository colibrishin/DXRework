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

	const std::vector<const Vector3*>& Collider::GetVertices() const
	{
		if (const auto mesh = m_mesh_.lock())
		{
			return mesh->GetVertices();
		}

		if (m_type_ == BOUNDING_TYPE_BOX)
		{
			return m_cube_stock_ref_;
		}
		else if (m_type_ == BOUNDING_TYPE_SPHERE)
		{
			return m_sphere_stock_ref_;
		}

		return {};
	}

	void Collider::Initialize()
	{
		InitializeStockVertices();
		if (const auto mesh = m_mesh_.lock())
		{
			GenerateFromMesh(mesh);
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

		m_previous_position_ = m_position_;
	}

	void Collider::InitializeStockVertices()
	{
		GeometricPrimitive::IndexCollection index;
		GeometricPrimitive::VertexCollection vertex;

		if (m_cube_stock_.empty())
		{
			GeometricPrimitive::CreateBox(vertex, index, Vector3::One, false);

			for (const auto& v : vertex)
			{
				m_cube_stock_.push_back(v.position);
			}

			for (const auto & v : m_cube_stock_)
			{
				m_cube_stock_ref_.push_back(&v);
			}
		}
		if (m_sphere_stock_.empty())
		{
			GeometricPrimitive::CreateSphere(vertex, index, 1.f, 16, false);

			for (const auto& v : vertex)
			{
				m_sphere_stock_.push_back(v.position);
			}

			for (const auto & v : m_sphere_stock_)
			{
				m_sphere_stock_ref_.push_back(&v);
			}
		}
	}

	inline void Collider::GenerateFromMesh(const WeakMesh& mesh)
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

		if (GetType() == BOUNDING_TYPE_BOX)
		{
			BoundingOrientedBox::CreateFromPoints(m_boundings_.box, mesh_obj->m_vertices_.size(), serialized_vertices.data(), sizeof(Vector3));
		}
		else if (GetType() == BOUNDING_TYPE_SPHERE)
		{
			BoundingSphere::CreateFromPoints(m_boundings_.sphere, mesh_obj->m_vertices_.size(), serialized_vertices.data(), sizeof(Vector3));
		}
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

		if (const auto mesh = m_mesh_.lock())
		{
			GenerateFromMesh(mesh);
		}

#ifdef _DEBUG
		GenerateDebugMesh();
#endif

		UpdateBoundings();
	}

	void Collider::SetMesh(const WeakMesh& mesh)
	{
		if (const auto locked = mesh.lock())
		{
			m_mesh_ = mesh;
			GenerateFromMesh(mesh);
		}
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

	void Collider::AddCollidedObject(const EntityID id)
	{
		m_collided_objects_.insert(id);

		if (!m_collision_count_.contains(id))
		{
			m_collision_count_.insert({ id, 1 });
		}
		else
		{
			m_collision_count_[id]++;
		}
	}

	void Collider::GetPenetration(const Collider& other, Vector3& normal, float& depth) const
	{
		auto dir = other.GetPosition() - GetPosition();
		dir.Normalize();

		Physics::GJK::GJKAlgorithm(*this, other, dir, normal, depth);
	}

	UINT Collider::GetCollisionCount(const EntityID id) const
	{
		if (!m_collision_count_.contains(id))
		{
			return 0;
		}

		return m_collision_count_.at(id);
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
			GetResourceManager().AddResource(L"CubeMesh", boost::make_shared<Mesh::CubeMesh>());
			m_debug_mesh_->AddResource(
				GetResourceManager().GetResource<Mesh::CubeMesh>(L"CubeMesh"));
		}
		else if (m_type_ == BOUNDING_TYPE_SPHERE)
		{
			GetResourceManager().AddResource(L"SphereMesh", boost::make_shared<Mesh::SphereMesh>());
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
		m_previous_position_ = m_position_;
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

		m_world_matrix_ = Matrix::CreateWorld(Vector3::Zero, g_forward, Vector3::Up);
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
