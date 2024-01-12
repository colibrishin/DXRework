#pragma once

namespace Engine::Physics
{
    using DirectX::SimpleMath::Vector3;
    using DirectX::SimpleMath::Quaternion;
    using DirectX::BoundingOrientedBox;
    using DirectX::BoundingSphere;
    using DirectX::BoundingBox;

    template <typename BoundingType>
    __forceinline static BoundingType __vectorcall TranslateBounding(
        const BoundingType& box, const Matrix& mat)
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
                m_boundings_.sphere.Radius = MaxElement(extents_);
            }
        }

        [[nodiscard]] Vector3 GetExtents() const
        {
            if (type == BOUNDING_TYPE_BOX)
            {
                return m_boundings_.box.Extents;
            }
            else if (type == BOUNDING_TYPE_SPHERE)
            {
                return Vector3(m_boundings_.sphere.Radius);
            }
            else
            {
                throw std::exception("GetExtents: Invalid type");
            }
        }

        [[nodiscard]] Quaternion GetOrientation() const
        {
            if (type == BOUNDING_TYPE_BOX)
            {
                return m_boundings_.box.Orientation;
            }
            else if (type == BOUNDING_TYPE_SPHERE)
            {
                return Quaternion::Identity;
            }
            else
            {
                throw std::exception("GetOrientation: Invalid type");
            }
        }

        void Transform(const Matrix& world)
        {
            if (type == BOUNDING_TYPE_BOX)
            {
                const auto translated = TranslateBounding(m_boundings_.box, world);
                m_boundings_.box = translated;
            }
            else if (type == BOUNDING_TYPE_SPHERE)
            {
                m_boundings_.sphere = TranslateBounding(m_boundings_.sphere, world);
            }
        }

        [[nodiscard]] bool __vectorcall Intersects(
            const GenericBounding& other, const Matrix& this_mat, const Matrix& other_mat, const Vector3& dir, const float epsilon = g_epsilon) const
        {
            if (type == BOUNDING_TYPE_BOX)
            {
                BoundingOrientedBox box = m_boundings_.box;
                box.Center             = box.Center + (dir * epsilon);
                box = TranslateBounding(box, this_mat);
                return Intersects(box, other, other_mat, dir);
            }
            else if (type == BOUNDING_TYPE_SPHERE)
            {
                BoundingSphere sphere = m_boundings_.sphere;
                sphere.Center         = sphere.Center + (dir * epsilon);
                sphere = TranslateBounding(sphere, this_mat);
                return Intersects(sphere, other, other_mat, dir);
            }
            else
            {
                throw std::exception("Intersects: Invalid type");
            }
        }

        [[nodiscard]] bool __vectorcall Intersects(
            const GenericBounding& other, const Matrix& this_mat, const Matrix& other_mat, const float epsilon = g_epsilon) const
        {
            if (type == BOUNDING_TYPE_BOX)
            {
                BoundingOrientedBox box = m_boundings_.box;
                box.Extents = box.Extents + Vector3(epsilon);
                box = TranslateBounding(box, this_mat);
                return Intersects(box, other, other_mat, epsilon);
            }
            else if (type == BOUNDING_TYPE_SPHERE)
            {
                BoundingSphere sphere = m_boundings_.sphere;
                sphere.Radius = sphere.Radius + epsilon;
                sphere = TranslateBounding(sphere, this_mat);
                return Intersects(sphere, other, other_mat, epsilon);
            }
            else
            {
                throw std::exception("Intersects: Invalid type");
            }
        }

        [[nodiscard]] bool __vectorcall TestRay(
            const GenericBounding& other, const Matrix& this_mat, const Matrix& other_mat, const Vector3& dir, float& dist) const
        {
            if (type == BOUNDING_TYPE_BOX)
            {
                BoundingOrientedBox box = m_boundings_.box;
                box = TranslateBounding(box, this_mat);
                return TestRay(m_boundings_.box, other, other_mat, dir, dist);
            }
            else if (type == BOUNDING_TYPE_SPHERE)
            {
                BoundingSphere sphere = m_boundings_.sphere;
                sphere = TranslateBounding(sphere, this_mat);
                return TestRay(m_boundings_.sphere, other, other_mat, dir, dist);
            }
            else
            {
                throw std::exception("Test: Invalid type");
            }
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
        void CreateFromPoints(size_t count, const Vector3* points, size_t stride)
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
            const BoundingTypeA& lhs, const BoundingTypeB& rhs, const Matrix& mat, const Vector3& dir, const float epsilon = g_epsilon)
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
                else if (rhs.type == BOUNDING_TYPE_SPHERE)
                {
                    BoundingSphere sphere = rhs.m_boundings_.sphere;
                    sphere.Center         = sphere.Center + (dir * epsilon);
                    sphere                = TranslateBounding(sphere, mat);
                    return lhs.Intersects(sphere);
                }
                else
                {
                    throw std::exception("Intersects: Invalid type");
                }
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
            const BoundingTypeA& lhs, const BoundingTypeB& rhs, const Matrix& other_mat, const Vector3& dir, float& dist)
        {
            if constexpr (std::is_same_v<BoundingTypeB, GenericBounding>)
            {
                if (rhs.type == BOUNDING_TYPE_BOX)
                {
                    BoundingOrientedBox box = rhs.m_boundings_.box;
                    box = TranslateBounding(box, other_mat);
                    return box.Intersects(Vector3(lhs.Center), dir, dist);
                }
                else if (rhs.type == BOUNDING_TYPE_SPHERE)
                {
                    BoundingSphere sphere = rhs.m_boundings_.sphere;
                    sphere = TranslateBounding(sphere, other_mat);
                    return sphere.Intersects(Vector3(lhs.Center), dir, dist);
                }
                else
                {
                    static_assert("Test: Invalid type");
                    throw std::exception("Intersects: Invalid type");
                }
            }
            else
            {
                return lhs.Intersects(rhs.Center, dir, dist);
            }
        }

        template <typename BoundingTypeA, typename BoundingTypeB>
        static bool __vectorcall Intersects(
            const BoundingTypeA& lhs, const BoundingTypeB& rhs, const Matrix& mat, const float epsilon = g_epsilon)
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
                else if (rhs.type == BOUNDING_TYPE_SPHERE)
                {
                    BoundingSphere sphere = rhs.m_boundings_.sphere;
                    sphere.Radius         = sphere.Radius + epsilon;
                    sphere                = TranslateBounding(sphere, mat);
                    return lhs.Intersects(sphere);
                }
                else
                {
                    throw std::exception("Intersects: Invalid type");
                }
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

            Boundings() { box = {}; }
        } m_boundings_;
    };
}
