#pragma once
#include <DirectXCollision.h>
#include <execution>

#include "egComponent.hpp"
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
		void SetCollided(const bool collided) { m_bCollided = collided; }

		bool Intersects(Collider& other) const;
		bool Contains(Collider& other) const;

		bool GetCollided() const { return m_bCollided; }
		bool HasCollisionStarted() const { return !m_bPreviousCollided && m_bCollided; }
		bool HasCollisionEnd() const { return m_bPreviousCollided && !m_bCollided; }

		void GenerateFromMesh(const std::weak_ptr<Resources::Mesh>& mesh);

		void Initialize() override;
		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Render() override;

	private:
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
				assert(false);
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
				assert(false);
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
				assert(false);
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
				assert(false);
			}

			throw std::exception("Invalid type");
		}

		void UpdateBoundings();

		bool m_bDirtyByTransform;
		bool m_bPreviousCollided;
		bool m_bCollided;

		Vector3 m_position_;
		Vector3 m_size_;
		Quaternion m_rotation_;

		eBoundingType m_type_;
		BoundingGroup m_boundings_;

	};

	inline Collider::Collider(const std::weak_ptr<Abstract::Object>& owner) : Component(owner),
	                                                                          m_bDirtyByTransform(false),
	                                                                          m_bPreviousCollided(false),
	                                                                          m_bCollided(false),
	                                                                          m_position_(Vector3::Zero),
	                                                                          m_size_(Vector3::One),
	                                                                          m_rotation_(Quaternion::Identity),
	                                                                          m_type_(BOUNDING_TYPE_BOX),
	                                                                          m_boundings_({})
	{
	}

	inline void Collider::Initialize()
	{
		if(const auto mesh = GetOwner().lock()->GetResource<Resources::Mesh>().lock())
		{
			GenerateFromMesh(mesh);
		}

		if (const auto tr = GetOwner().lock()->GetComponent<Transform>().lock(); m_bDirtyByTransform)
		{
			m_position_ = tr->GetPosition();
			m_size_ = tr->GetScale();
			m_rotation_ = tr->GetRotation();

			UpdateBoundings();
		}
	}

	inline void Collider::PreUpdate()
	{
		m_bPreviousCollided = m_bCollided;
	}

	inline void Collider::Update()
	{
		if (const auto tr = GetOwner().lock()->GetComponent<Transform>().lock(); m_bDirtyByTransform)
		{
			m_position_ = tr->GetPosition();
			m_size_ = tr->GetScale();
			m_rotation_ = tr->GetRotation();

			UpdateBoundings();
		}
	}

	inline void Collider::PreRender()
	{
	}

	inline void Collider::Render()
	{
	}
}
