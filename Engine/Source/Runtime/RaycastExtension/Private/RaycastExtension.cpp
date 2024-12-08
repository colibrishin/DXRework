#include "../Public/RaycastExtension.hpp"

#include "Source/Runtime/Core/Components/Collider/Public/Collider.hpp"
#include "Source/Runtime/Core/GenericBounding/Public/GenericBounding.hpp"

bool Engine::Physics::RaycastExtension::Intersects(
	const Weak<Components::Collider>& collider,
	const Vector3& start,
	const Vector3& dir,
	float distance,
	float& intersection
)
{
	if (const Strong<Components::Collider>& locked = collider.lock())
	{
		if (locked->GetType() == BOUNDING_TYPE_BOX)
		{
			const auto    box     = locked->GetBounding<BoundingOrientedBox>();
			const Vector3 Extents = box.Extents;
			const auto    test    = Physics::RaycastExtension::TestRayOBBIntersection
					(
					 start, dir, -Extents,
					 Extents,
					 locked->GetWorldMatrix(),
					 intersection
					);

			return test && intersection <= distance;
		}

		if (locked->GetType() == BOUNDING_TYPE_SPHERE)
		{
			const auto sphere = locked->GetBounding<BoundingSphere>();
			const auto test   = Physics::RaycastExtension::TestRaySphereIntersection
					(
					 start, dir, sphere.Center,
					 sphere.Radius,
					 intersection
					);

			return test && intersection <= distance;
		}
	}
	
	return false;
}
