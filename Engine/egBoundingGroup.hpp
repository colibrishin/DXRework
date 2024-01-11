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

        Vector3 GetExtents() const
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

        void Transform(const Matrix& world)
        {
            if (type == BOUNDING_TYPE_BOX)
            {
                m_boundings_.box = TranslateBounding(m_boundings_.box, world);
            }
            else if (type == BOUNDING_TYPE_SPHERE)
            {
                m_boundings_.sphere = TranslateBounding(m_boundings_.sphere, world);
            }
        }

        template <typename BoundingType>
        DirectX::ContainmentType __vectorcall ContainsBy(const BoundingType& other) const
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

    private:
        friend class boost::serialization::access;
        template <class Archive>
        void serialize(Archive& ar, const unsigned int file_version)
        {
            ar & m_boundings_.box;
            ar & m_boundings_.sphere;
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
