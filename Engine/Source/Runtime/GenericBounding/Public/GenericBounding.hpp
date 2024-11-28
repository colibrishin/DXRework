#pragma once
#include <boost/serialization/access.hpp>

#include <directxtk12/SimpleMath.h>

#include "Source/Runtime/MathExtension/Public/MathExtension.hpp"

namespace Engine
{
	enum eBoundingType
	{
		BOUNDING_TYPE_BOX = 0,
		BOUNDING_TYPE_SPHERE,
	};

	using DirectX::SimpleMath::Vector3;
	using DirectX::SimpleMath::Quaternion;
    using DirectX::SimpleMath::Matrix;
	using DirectX::BoundingOrientedBox;
	using DirectX::BoundingSphere;
	using DirectX::BoundingBox;

	template <typename BoundingType>
	__forceinline static BoundingType __vectorcall TranslateBounding(
		const BoundingType& box, const Matrix& mat
	)
	{
		if constexpr (std::is_same_v<BoundingType, BoundingOrientedBox>)
		{
			auto box_ = const_cast<BoundingOrientedBox&>(box);
			box_.Transform(box_, mat);
			return box_;
		}
		else if constexpr (std::is_same_v<BoundingType, BoundingSphere>)
		{
			auto sphere_ = const_cast<BoundingSphere&>(box);
			sphere_.Transform(sphere_, mat);
			return sphere_;
		}
		else
		{
			static_assert("TranslateBounding: Invalid type");
			return {};
		}
	}

	template <float Epsilon = 0.0001f>
	struct GenericBounding
	{
	public:
		GenericBounding()
			: type(BOUNDING_TYPE_BOX),
			  m_boundings_({}) {}

		GenericBounding(const GenericBounding& other)
		{
			type = other.type;
			if (type == BOUNDING_TYPE_BOX)
			{
				m_boundings_.box = other.m_boundings_.box;
			}
			else if (type == BOUNDING_TYPE_SPHERE)
			{
				m_boundings_.sphere = other.m_boundings_.sphere;
			}
		}

		template <typename BoundingType>
		__forceinline BoundingType __vectorcall As(const Matrix& mat) const
		{
			if constexpr (std::is_same_v<BoundingType, BoundingOrientedBox>)
			{
				const auto box_ = TranslateBounding(m_boundings_.box, mat);
				return box_;
			}
			else if constexpr (std::is_same_v<BoundingType, BoundingSphere>)
			{
				const auto sphere_ = TranslateBounding(m_boundings_.sphere, mat);
				return sphere_;
			}
			else
			{
				static_assert("As: Invalid type");
				return {};
			}
		}

		void SetType(const eBoundingType type_)
		{
			type = type_;
		}

		void SetCenter(const Vector3& center_)
		{
			if (type == BOUNDING_TYPE_BOX)
			{
				m_boundings_.box.Center = center_;
			}
			else if (type == BOUNDING_TYPE_SPHERE)
			{
				m_boundings_.sphere.Center = center_;
			}
		}

		void SetExtents(const Vector3& extents_)
		{
			if (type == BOUNDING_TYPE_BOX)
			{
				m_boundings_.box.Extents = extents_;
			}
			else if (type == BOUNDING_TYPE_SPHERE)
			{
				m_boundings_.sphere.Radius = MathExtension::MaxElement(extents_);
			}
		}

		[[nodiscard]] Vector3 GetExtents() const
		{
			if (type == BOUNDING_TYPE_BOX)
			{
				return m_boundings_.box.Extents;
			}
			if (type == BOUNDING_TYPE_SPHERE)
			{
				return Vector3(m_boundings_.sphere.Radius);
			}
			throw std::exception("GetExtents: Invalid type");
		}

		[[nodiscard]] Quaternion GetOrientation() const
		{
			if (type == BOUNDING_TYPE_BOX)
			{
				return m_boundings_.box.Orientation;
			}
			if (type == BOUNDING_TYPE_SPHERE)
			{
				return Quaternion::Identity;
			}
			throw std::exception("GetOrientation: Invalid type");
		}

		void Transform(const Matrix& world)
		{
			if (type == BOUNDING_TYPE_BOX)
			{
				const auto translated = TranslateBounding(m_boundings_.box, world);
				m_boundings_.box      = translated;
			}
			else if (type == BOUNDING_TYPE_SPHERE)
			{
				m_boundings_.sphere = TranslateBounding(m_boundings_.sphere, world);
			}
		}

		[[nodiscard]] bool __vectorcall ContainsBy(
			const GenericBounding& other, const Matrix& this_mat, const Matrix& other_mat
		) const
		{
			if (other.type == BOUNDING_TYPE_BOX)
			{
				BoundingOrientedBox box = other.m_boundings_.box;
				box                     = TranslateBounding(box, other_mat);
				return ContainsBy(box);
			}
			if (other.type == BOUNDING_TYPE_SPHERE)
			{
				BoundingSphere sphere = other.m_boundings_.sphere;
				sphere                = TranslateBounding(sphere, other_mat);
				return ContainsBy(sphere);
			}

			return false;
		}

		template <typename T>
		[[nodiscard]] bool __vectorcall Intersects(const T& bounding) const
		{
			if (type == BOUNDING_TYPE_BOX)
			{
				return m_boundings_.box.Intersects(bounding);
			}
			if (type == BOUNDING_TYPE_SPHERE)
			{
				return m_boundings_.sphere.Intersects(bounding);
			}

			throw std::exception("Intersects: Invalid type");
		}

		[[nodiscard]] bool __vectorcall Intersects(
			const GenericBounding& other, const Matrix& this_mat, const Matrix& other_mat, const Vector3& dir,
			const float            epsilon = Epsilon
		) const
		{
			if (type == BOUNDING_TYPE_BOX)
			{
				BoundingOrientedBox box = m_boundings_.box;
				box.Center              = box.Center + (dir * epsilon);
				box                     = TranslateBounding(box, this_mat);
				return Intersects(box, other, other_mat, dir);
			}
			if (type == BOUNDING_TYPE_SPHERE)
			{
				BoundingSphere sphere = m_boundings_.sphere;
				sphere.Center         = sphere.Center + (dir * epsilon);
				sphere                = TranslateBounding(sphere, this_mat);
				return Intersects(sphere, other, other_mat, dir);
			}
			throw std::exception("Intersects: Invalid type");
		}

		[[nodiscard]] bool __vectorcall Intersects(
			const GenericBounding& other, const Matrix& this_mat, const Matrix& other_mat,
			const float            epsilon = Epsilon
		) const
		{
			if (type == BOUNDING_TYPE_BOX)
			{
				BoundingOrientedBox box = m_boundings_.box;
				box.Extents             = box.Extents + Vector3(epsilon);
				box                     = TranslateBounding(box, this_mat);
				return Intersects(box, other, other_mat, epsilon);
			}
			if (type == BOUNDING_TYPE_SPHERE)
			{
				BoundingSphere sphere = m_boundings_.sphere;
				sphere.Radius         = sphere.Radius + epsilon;
				sphere                = TranslateBounding(sphere, this_mat);
				return Intersects(sphere, other, other_mat, epsilon);
			}
			throw std::exception("Intersects: Invalid type");
		}

		[[nodiscard]] bool __vectorcall TestRay(const GenericBounding& other, const Vector3& dir, float& dist) const
		{
			if (type == BOUNDING_TYPE_BOX)
			{
				BoundingOrientedBox box = m_boundings_.box;
				return TestRay(m_boundings_.box, other, dir, dist);
			}
			if (type == BOUNDING_TYPE_SPHERE)
			{
				BoundingSphere sphere = m_boundings_.sphere;
				return TestRay(m_boundings_.sphere, other, dir, dist);
			}
			throw std::exception("Test: Invalid type");
		}

		[[nodiscard]] bool __vectorcall TestRay(const Vector3& center, const Vector3& dir, float& dist) const
		{
			if (type == BOUNDING_TYPE_BOX)
			{
				return m_boundings_.box.Intersects(center, dir, dist);
			}
			if (type == BOUNDING_TYPE_SPHERE)
			{
				return m_boundings_.sphere.Intersects(center, dir, dist);
			}
			throw std::exception("Test: Invalid type");
		}

		[[nodiscard]] bool __vectorcall Contains(const Vector3& point) const
		{
			if (type == BOUNDING_TYPE_BOX)
			{
				return m_boundings_.box.Contains(point);
			}
			if (type == BOUNDING_TYPE_SPHERE)
			{
				return m_boundings_.sphere.Contains(point);
			}
			throw std::exception("Contains: Invalid type");
		}

		template <typename BoundingType>
		[[nodiscard]] DirectX::ContainmentType __vectorcall ContainsBy(const BoundingType& other) const
		{
			if constexpr (std::is_same_v<BoundingType, BoundingOrientedBox>)
			{
				return other.Contains(m_boundings_.box);
			}
			else if constexpr (std::is_same_v<BoundingType, BoundingSphere>)
			{
				return other.Contains(m_boundings_.sphere);
			}
			else if constexpr (std::is_same_v<BoundingType, BoundingBox>)
			{
				return other.Contains(m_boundings_.box);
			}
			else
			{
				static_assert("ContainsBy: Invalid type");
				return DirectX::ContainmentType::DISJOINT;
			}
		}

		void __vectorcall UpdateFromBoundingBox(const BoundingOrientedBox& box_)
		{
			m_boundings_.box = box_;
		}

		template <typename BoundingType>
		void __vectorcall CreateFromPoints(const std::size_t count, const Vector3* points, const std::size_t stride)
		{
			if (std::is_same_v<BoundingType, BoundingOrientedBox>)
			{
				m_boundings_.box.CreateFromPoints(m_boundings_.box, count, points, stride);
			}
			else if (std::is_same_v<BoundingType, BoundingSphere>)
			{
				m_boundings_.sphere.CreateFromPoints(m_boundings_.sphere, count, points, stride);
			}
			else if (std::is_same_v<BoundingType, BoundingBox>)
			{
				BoundingBox box_;
				BoundingBox::CreateFromPoints(box_, count, points, stride);
				BoundingOrientedBox::CreateFromBoundingBox(m_boundings_.box, box_);
			}
		}

		[[nodiscard]] __forceinline GenericBounding __vectorcall Transform(const Matrix& mat) const
		{
			GenericBounding ret = *this;
			ret.Transform(mat);
			return ret;
		}

		__forceinline void Translate(const Vector3& v)
		{
			if (type == BOUNDING_TYPE_BOX)
			{
				m_boundings_.box.Center = m_boundings_.box.Center + v;
			}
			else if (type == BOUNDING_TYPE_SPHERE)
			{
				m_boundings_.sphere.Center = m_boundings_.sphere.Center + v;
			}
		}

		[[nodiscard]] float Distance(const Vector3& point) const
		{
			return std::sqrtf(Vector3::DistanceSquared(m_boundings_.box.Center, point));
		}

	private:
		friend class boost::serialization::access;

		template <class Archive>
		void serialize(Archive& ar, const unsigned int file_version)
		{
			ar & m_boundings_.box;
			ar & m_boundings_.sphere;
		}

		template <typename BoundingTypeA, typename BoundingTypeB>
		static bool __vectorcall Intersects(
			const BoundingTypeA& lhs, const BoundingTypeB& rhs, const Matrix& mat, const Vector3& dir,
			const float          epsilon = Epsilon
		)
		{
			if constexpr (std::is_same_v<BoundingTypeB, GenericBounding>)
			{
				if (rhs.type == BOUNDING_TYPE_BOX)
				{
					BoundingOrientedBox box = rhs.m_boundings_.box;
					box.Center              = box.Center + (dir * epsilon);
					box                     = TranslateBounding(box, mat);
					return lhs.Intersects(box);
				}
				if (rhs.type == BOUNDING_TYPE_SPHERE)
				{
					BoundingSphere sphere = rhs.m_boundings_.sphere;
					sphere.Center         = sphere.Center + (dir * epsilon);
					sphere                = TranslateBounding(sphere, mat);
					return lhs.Intersects(sphere);
				}
				throw std::exception("Intersects: Invalid type");
			}
			else
			{
				BoundingTypeB rhs_ = rhs;
				rhs_.Center        = rhs_.Center + (dir * epsilon);
				rhs_               = TranslateBounding(rhs_, mat);
				return lhs.Intersects(rhs);
			}
		}

		template <typename BoundingTypeA, typename BoundingTypeB>
		static bool __vectorcall TestRay(
			const BoundingTypeA& lhs, const BoundingTypeB& rhs, const Vector3& dir, float& dist
		)
		{
			if constexpr (std::is_same_v<BoundingTypeB, GenericBounding>)
			{
				if (rhs.type == BOUNDING_TYPE_BOX)
				{
					BoundingOrientedBox box = rhs.m_boundings_.box;
					return box.Intersects(Vector3(lhs.Center), dir, dist);
				}
				if (rhs.type == BOUNDING_TYPE_SPHERE)
				{
					BoundingSphere sphere = rhs.m_boundings_.sphere;
					return sphere.Intersects(Vector3(lhs.Center), dir, dist);
				}
				static_assert("Test: Invalid type");
				throw std::exception("Intersects: Invalid type");
			}
			else
			{
				return lhs.Intersects(rhs.Center, dir, dist);
			}
		}

		template <typename BoundingTypeA, typename BoundingTypeB>
		static bool __vectorcall Intersects(
			const BoundingTypeA& lhs, const BoundingTypeB& rhs, const Matrix& mat, const float epsilon = Epsilon
		)
		{
			if constexpr (std::is_same_v<BoundingTypeB, GenericBounding>)
			{
				if (rhs.type == BOUNDING_TYPE_BOX)
				{
					BoundingOrientedBox box = rhs.m_boundings_.box;
					box.Extents             = box.Extents + Vector3(epsilon);
					box                     = TranslateBounding(box, mat);
					return lhs.Intersects(box);
				}
				if (rhs.type == BOUNDING_TYPE_SPHERE)
				{
					BoundingSphere sphere = rhs.m_boundings_.sphere;
					sphere.Radius         = sphere.Radius + epsilon;
					sphere                = TranslateBounding(sphere, mat);
					return lhs.Intersects(sphere);
				}
				throw std::exception("Intersects: Invalid type");
			}
			else
			{
				BoundingTypeB rhs_ = rhs;
				rhs_.Extents       = rhs_.Extents + Vector3(epsilon);
				rhs_               = TranslateBounding(rhs_, mat);
				return lhs.Intersects(rhs_);
			}
		}

		eBoundingType type;

		union Boundings
		{
			BoundingOrientedBox box;
			BoundingSphere      sphere;

			Boundings()
			{
				box = {};
			}
		} m_boundings_;
	};
}
