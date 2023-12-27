#pragma once
#include "egType.h"

namespace Engine::Physics
{
    using DirectX::SimpleMath::Vector3;
    using DirectX::SimpleMath::Quaternion;
    using DirectX::BoundingOrientedBox;
    using DirectX::BoundingSphere;
    using DirectX::BoundingBox;

    template <typename BoundingType>
    __forceinline static BoundingType __vectorcall TranslateBounding(
        const BoundingType& box, const Vector3& scale, const Quaternion& rotation, const Vector3& position)
    {
        if constexpr (std::is_same_v<BoundingType, BoundingOrientedBox>)
        {
            auto box_ = const_cast<BoundingOrientedBox&>(box);
            box_.Transform(box_, 1.f, rotation, position);
            box_.Extents = Vector3(box_.Extents.x * scale.x, box_.Extents.y * scale.y, box_.Extents.z * scale.z);
            return box_;
        }
        else if constexpr (std::is_same_v<BoundingType, BoundingSphere>)
        {
            auto sphere_ = const_cast<BoundingSphere&>(box);
            sphere_.Transform(sphere_, MaxElement(scale), rotation, position);
            return sphere_;
        }
        else
        {
            static_assert("TranslateBounding: Invalid type");
            return {};
        }
    }

    union BoundingGroup
    {
    public:
        BoundingGroup() : box(Vector3::Zero, {0.5f, 0.5f, 0.5f}, Quaternion::Identity) {}

        template <typename BoundingType>
        __forceinline BoundingType __vectorcall As(const Vector3& scale, const Quaternion& rotation, const Vector3& translation) const
        {
            if constexpr (std::is_same_v<BoundingType, BoundingOrientedBox>)
            {
                const auto box_ = TranslateBounding(box, scale, rotation, translation);
                return box_;
            }
            else if constexpr (std::is_same_v<BoundingType, BoundingSphere>)
            {
                const auto sphere_ = TranslateBounding(sphere, scale, rotation, translation);
                return sphere_;
            }
            else
            {
                static_assert("TranslateBounding: Invalid type");
                return {};
            }
        }

        void __vectorcall UpdateFromBoundingBox(const BoundingBox& box_)
        {
            BoundingOrientedBox::CreateFromBoundingBox(box, box_);
        }

        template <typename BoundingType>
        void CreateFromPoints(size_t count, const Vector3* points, size_t stride)
        {
            if (std::is_same_v<BoundingType, BoundingOrientedBox>)
            {
                box.CreateFromPoints(box, count, points, stride);
            }
            else if (std::is_same_v<BoundingType, BoundingSphere>)
            {
                sphere.CreateFromPoints(sphere, count, points, stride);
            }
            else if (std::is_same_v<BoundingType, BoundingBox>)
            {
                BoundingBox box_;
                BoundingBox::CreateFromPoints(box_, count, points, stride);
                BoundingOrientedBox::CreateFromBoundingBox(box, box_);
            }
            else
            {
                static_assert("TranslateBounding: Invalid type");
            }
        }

    private:
        BoundingOrientedBox box;
        BoundingSphere      sphere;
    };
}
