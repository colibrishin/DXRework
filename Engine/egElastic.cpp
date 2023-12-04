#include "pch.hpp"

#include "egCollider.hpp"
#include "egElastic.h"
#include "egPhysics.h"

#undef max
#undef min

namespace Engine::Physics
{
	void EvalImpulse(const Vector3& pos1, const Vector3& pos2, const Vector3& point, float penetration, const Vector3& normal, float invm1, float invm2, const Vector3& rv1, const Vector3& rv2, const Vector3& vel1, const Vector3& vel2, const XMFLOAT3X3& inertiaT1, const XMFLOAT3X3& inertiaT2, Vector3& linear1, Vector3& linear2, Vector3& angular1, Vector3& angular2, Vector3& penetrationA, Vector3& penetrationB)
	{
		const float total_mass = invm1 + invm2;

		const float penetration1 = (penetration * (invm1 / total_mass));
		const float penetration2 = (penetration * (invm2 / total_mass));

		penetrationA = -normal * penetration1;
		penetrationB = normal * penetration2;

		const Vector3 rel1 = point - pos1;
		const Vector3 rel2 = point - pos2;

		const Vector3 angVel1 = rv1.Cross(rel1);
		const Vector3 angVel2 = rv2.Cross(rel1);

		const Vector3 sumVel1 = vel1 + angVel1;
		const Vector3 sumVel2 = vel2 + angVel2;

		const Vector3 contact_vel = sumVel2 - sumVel1;

		const float impulse_force = contact_vel.Dot(normal);

		const Vector3 inertia1 = (XMTensorCross(inertiaT1, rel1.Cross(normal)).Cross(rel1));
		const Vector3 inertia2 = (XMTensorCross(inertiaT2, rel2.Cross(normal)).Cross(rel2));

		const float angular_impulse = (inertia1 + inertia2).Dot(normal);

		const float j = (-(1.0f + g_restitution_coefficient) * impulse_force) / (total_mass + angular_impulse);
		const Vector3 impulse = normal * j;

		linear1 = -impulse;
		linear2 = impulse;

		angular1 = rel1.Cross(-impulse);
		angular2 = rel2.Cross(impulse);
	}
}
