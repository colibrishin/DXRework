#pragma once
#include <DirectXCollision.h>
#include <execution>

#include "egPhysics.h"
#include "egComponent.hpp"
#include "egHelper.hpp"
#include "egMesh.hpp"
#include "egTransform.hpp"
#include "egObject.hpp"

namespace Engine::Component
{
	using namespace DirectX;

	class Collider : public Abstract::Component
	{
	public:
		Collider(const std::weak_ptr<Abstract::Object>& owner);
		~Collider() override = default;

		void SetDirtyWithTransform(const bool dirty) { m_bDirtyByTransform = dirty; }

		void SetPosition(const Vector3& position);
		void SetRotation(const Quaternion& rotation);
		void SetSize(const Vector3& size);
		void SetType(const eBoundingType type);
		void SetMass(const float mass) { m_mass_ = mass; }

		bool Intersects(Collider& other) const;
		bool Contains(Collider& other) const;
		bool Intersects(const Ray& other, float dist) const;

		void AddCollidedObject(const uint64_t id) { m_collided_objects_.insert(id); }
		void RemoveCollidedObject(const uint64_t id) { m_collided_objects_.erase(id); }
		bool IsCollidedObject(const uint64_t id) const { return m_collided_objects_.contains(id); }
		const std::set<uint64_t>& GetCollidedObjects() const { return m_collided_objects_; }

		Vector3 GetPosition() const { return m_position_; }
		Quaternion GetRotation() const { return m_rotation_; }
		Vector3 GetSize() const { return m_size_; }
		void GetPenetration(Collider& other, Vector3& normal, float& depth) const;
		float GetMass() const { return m_mass_; }
		float GetInverseMass() const { return 1.0f / m_mass_; }
		XMFLOAT3X3 GetInertiaTensor() const { return m_inertia_tensor_; }

		Vector3 GetSupportPoint(const Vector3& dir) const;

		void GenerateFromMesh(const std::weak_ptr<Resources::Mesh>& mesh);

		void Initialize() override;
		void UpdateDataFromTransform();
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void FixedUpdate(const float& dt) override;

	private:
		union BoundingGroup
		{
			DirectX::BoundingOrientedBox box;
			DirectX::BoundingSphere sphere;
			DirectX::BoundingFrustum frustum;
		};

		template <typename T>
		bool Intersects_GENERAL_TYPE(const T& other)
		{
			if (m_type_ == BOUNDING_TYPE_BOX)
			{
				return As<BoundingOrientedBox>().Intersects(other);
			}
			else if (m_type_ == BOUNDING_TYPE_SPHERE)
			{
				return As<BoundingSphere>().Intersects(other);
			}
			else if (m_type_ == BOUNDING_TYPE_FRUSTUM)
			{
				throw std::exception("Not implemented");
			}

			return false;
		}

		template <typename T>
		bool Contains_GENERAL_TYPE(const T& other)
		{
			if (m_type_ == BOUNDING_TYPE_BOX)
			{
				return As<BoundingOrientedBox>().Contains(other);
			}
			else if (m_type_ == BOUNDING_TYPE_SPHERE)
			{
				return As<BoundingSphere>().Contains(other);
			}
			else if (m_type_ == BOUNDING_TYPE_FRUSTUM)
			{
				throw std::exception("Not implemented");
			}

			return false;
		}

		template <typename T>
		static void SetSize_GENERAL_TYPE(T& value, const Vector3& size)
		{
			if constexpr (std::is_same_v<T, BoundingOrientedBox>)
			{
				value.Extents = size / 2;
			}
			else if constexpr (std::is_same_v<T, BoundingSphere>)
			{
				value.Radius = size.x / 2;
			}
			else if constexpr (std::is_same_v<T, BoundingFrustum>)
			{
				throw std::exception("Not implemented");
			}
			else
			{
				throw std::exception("Invalid type");
			}
		}

		template <typename T>
		void SetRotation_GENERAL_TYPE(T& value, const Quaternion& rotation)
		{
			if constexpr (std::is_same_v<T, BoundingOrientedBox>)
			{
				value.Orientation = rotation;
			}
			else if constexpr (std::is_same_v<T, BoundingSphere>)
			{
				return;
			}
			else if constexpr (std::is_same_v<T, BoundingFrustum>)
			{
				assert(false);
			}
			else
			{
				throw std::exception("Invalid type");
			}
		}

		template <typename T>
		void SetPosition_GENERAL_TYPE(T& value, const Vector3& position)
		{
			if constexpr (std::is_same_v<T, BoundingFrustum>)
			{
				value.Origin = position;
				return;
			}

			As<T>().Center = position;
		}

		template <typename T>
		T& As() 
		{
			if constexpr (std::is_same_v<T, BoundingOrientedBox>)
			{
				return m_boundings_.box;
			}
			else if constexpr (std::is_same_v<T, BoundingSphere>)
			{
				return m_boundings_.sphere;
			}
			else if constexpr (std::is_same_v<T, BoundingFrustum>)
			{
				throw std::exception("Not implemented");
			}

			throw std::exception("Invalid type");
		}

		template <typename T>
		void GetPenetration_GENERAL_TYPE(const T& value, Vector3& normal, float& penetration)
		{
			if (m_type_ == BOUNDING_TYPE_BOX)
			{
				penetration = Physics::GetCollisionPenetrationDepth(As<BoundingOrientedBox>(), value, normal);
			}
			else if (m_type_ == BOUNDING_TYPE_SPHERE)
			{
				penetration = Physics::GetCollisionPenetrationDepth(value, As<BoundingSphere>(), normal);
			}
			else if (m_type_ == BOUNDING_TYPE_FRUSTUM)
			{
				throw std::exception("Not implemented");
			}
		}

		void UpdateBoundings();
		void UpdateInertiaTensor();
		void GenerateInertiaCube();
		void GenerateInertiaSphere();

	private:
		std::set<uint64_t> m_collided_objects_;

		bool m_bDirtyByTransform;

		Vector3 m_position_;
		Vector3 m_size_;
		Quaternion m_rotation_;

		eBoundingType m_type_;
		BoundingGroup m_boundings_;

		float m_mass_;
		Vector3 m_inverse_inertia_;
		XMFLOAT3X3 m_inertia_tensor_;

	};

	inline Collider::Collider(const std::weak_ptr<Abstract::Object>& owner) : Component(COMPONENT_PRIORITY_COLLIDER,
																				owner),
																			m_bDirtyByTransform(false),
																			m_position_(Vector3::Zero),
																			m_size_(Vector3::One),
																			m_rotation_(Quaternion::Identity),
																			m_type_(BOUNDING_TYPE_BOX),
																			m_boundings_({}), m_mass_(1.0f),
																			m_inertia_tensor_()
	{
	}

	inline void Collider::GenerateInertiaCube()
	{
		const Vector3 dimensions_squared = GetSize() * GetSize();

		m_inverse_inertia_.x = (12.0f * GetInverseMass()) / (dimensions_squared.y + dimensions_squared.z);
		m_inverse_inertia_.y = (12.0f * GetInverseMass()) / (dimensions_squared.x + dimensions_squared.z);
		m_inverse_inertia_.z = (12.0f * GetInverseMass()) / (dimensions_squared.x + dimensions_squared.y);
	}

	inline void Collider::GenerateInertiaSphere()
	{
		const float radius = GetSize().x;
		const float i = 2.5f * GetInverseMass() / (radius*radius);

		m_inverse_inertia_ = Vector3(i, i, i);
	}

	inline void Collider::Initialize()
	{
		if(const auto mesh = GetOwner().lock()->GetResource<Resources::Mesh>().lock())
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
		else if (m_type_ == BOUNDING_TYPE_FRUSTUM)
		{
			throw std::exception("Not implemented");
		}

		UpdateInertiaTensor();
		UpdateDataFromTransform();
	}

	inline void Collider::UpdateDataFromTransform()
	{
		if (const auto tr = GetOwner().lock()->GetComponent<Transform>().lock(); m_bDirtyByTransform)
		{
			m_position_ = tr->GetPosition();
			m_size_ = tr->GetScale();
			m_rotation_ = tr->GetRotation();

			UpdateBoundings();
		}
	}

	inline void Collider::PreUpdate(const float& dt)
	{
		UpdateDataFromTransform();
		UpdateInertiaTensor();
	}

	inline void Collider::Update(const float& dt)
	{
		UpdateDataFromTransform();
		UpdateInertiaTensor();
	}

	inline void Collider::PreRender(const float dt)
	{
	}

	inline void Collider::Render(const float dt)
	{
	}
}
