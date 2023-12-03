#pragma once
#include <SimpleMath.h>
#include <DirectXCollision.h>
#include <execution>

#include "egPhysics.h"
#include "egComponent.hpp"
#include "egDebugObject.hpp"
#include "egHelper.hpp"
#include "egMesh.hpp"
#include "egTransform.hpp"
#include "egObject.hpp"

namespace Engine::Manager
{
	class Debugger;
}

namespace Engine::Component
{
	using namespace DirectX;

	class Collider : public Abstract::Component
	{
	public:
		Collider(const WeakObject& owner, const WeakMesh& mesh = {});
		~Collider() override = default;

		void SetDirtyWithTransform(const bool dirty)
		{
			m_bDirtyByTransform = dirty;
			if (dirty)
			{
				UpdateFromTransform();
			}
		}

		void SetPosition(const Vector3& position);
		void SetRotation(const Quaternion& rotation);
		void SetSize(const Vector3& size);
		void SetType(const eBoundingType type);
		void SetMass(const float mass) { m_mass_ = mass; }
		void SetMesh(const WeakMesh& mesh);

		bool Intersects(Collider& other) const;
		bool Intersects(const Ray& ray, float distance, float& intersection) const;
		bool Contains(Collider& other) const;

		void AddCollidedObject(const EntityID id);
		void AddSpeculationObject(const EntityID id) { m_speculative_collision_candidates_.insert(id); }

		void RemoveCollidedObject(const EntityID id) { m_collided_objects_.erase(id); }
		void RemoveSpeculationObject(const EntityID id) { m_speculative_collision_candidates_.erase(id); }

		bool IsCollidedObject(const EntityID id) const { return m_collided_objects_.contains(id); }
		const std::set<EntityID>& GetCollidedObjects() const { return m_collided_objects_; }
		const std::set<EntityID>& GetSpeculation() const { return m_speculative_collision_candidates_; }

		bool GetDirtyFlag() const { return m_bDirtyByTransform; }

		Vector3 GetPreviousPosition() const { return m_previous_position_; }
		Vector3 GetPosition() const { return m_position_; }
		Quaternion GetRotation() const { return m_rotation_; }
		Vector3 GetSize() const { return m_size_; }
		void GetPenetration(const Collider& other, Vector3& normal, float& depth) const;
		UINT GetCollisionCount(const EntityID id) const;

		float GetMass() const { return m_mass_; }
		float GetInverseMass() const { return 1.0f / m_mass_; }
		XMFLOAT3X3 GetInertiaTensor() const { return m_inertia_tensor_; }

		eBoundingType GetType() const { return m_type_; }

		virtual const std::vector<const Vector3*>& GetVertices() const;
		const Matrix& GetWorldMatrix() const { return m_world_matrix_; }

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void FixedUpdate(const float& dt) override;

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

			throw std::exception("Invalid type");
		}

	private:
		friend class boost::serialization::access;
		static void InitializeStockVertices();
		void GenerateFromMesh(const WeakMesh& mesh);

		union BoundingGroup
		{
			DirectX::BoundingOrientedBox box;
			DirectX::BoundingSphere sphere;
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
		}

		template <typename T>
		void SetPosition_GENERAL_TYPE(T& value, const Vector3& position)
		{
			As<T>().Center = position;
		}

		void UpdateFromTransform();
		void UpdateBoundings();
		void UpdateInertiaTensor();
		void GenerateInertiaCube();
		void GenerateInertiaSphere();

		inline static std::vector<Vector3> m_cube_stock_ = {};
		inline static std::vector<Vector3> m_sphere_stock_ = {};

		inline static std::vector<const Vector3*> m_cube_stock_ref_ = {};
		inline static std::vector<const Vector3*> m_sphere_stock_ref_ = {};

		std::set<EntityID> m_collided_objects_;
		std::map<EntityID, UINT> m_collision_count_;
		std::set<EntityID> m_speculative_collision_candidates_;

		bool m_bDirtyByTransform;

#ifdef _DEBUG
		void GenerateDebugMesh();
		void AfterDeserialized() override;

		boost::shared_ptr<Object::DebugObject> m_debug_mesh_;
#endif

		Vector3 m_previous_position_;
		Vector3 m_position_;
		Vector3 m_size_;
		Quaternion m_rotation_;

		eBoundingType m_type_;
		BoundingGroup m_boundings_;

		float m_mass_;
		Vector3 m_inverse_inertia_;
		XMFLOAT3X3 m_inertia_tensor_;
		Matrix m_world_matrix_;
		WeakMesh m_mesh_;

	};

	inline Collider::Collider(const WeakObject& owner, const WeakMesh& mesh) : Component(COMPONENT_PRIORITY_COLLIDER,
																				owner),
																			m_bDirtyByTransform(false),
																			m_position_(Vector3::Zero),
																			m_size_(Vector3::One),
																			m_rotation_(Quaternion::Identity),
																			m_type_(BOUNDING_TYPE_BOX),
																			m_boundings_({}), m_mass_(1.0f),
																			m_inertia_tensor_(),
																			m_mesh_(mesh)
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

	inline void Collider::PreUpdate(const float& dt)
	{
		static float second_counter = 0.f;

		if (second_counter >= 1.f)
		{
			m_collision_count_.clear();
		}

		second_counter += dt;

		UpdateFromTransform();
		UpdateInertiaTensor();
	}

	inline void Collider::Update(const float& dt)
	{
		UpdateFromTransform();
		UpdateInertiaTensor();
	}

	inline void Collider::PreRender(const float dt)
	{
#ifdef _DEBUG
		if (const auto tr = m_debug_mesh_->GetComponent<Transform>().lock())
		{
			tr->SetPosition(GetPosition());
			tr->SetRotation(GetRotation());
			tr->SetScale(GetSize());
		}
#endif
	}
}
