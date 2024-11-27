#pragma once

namespace Engine::Physics
{
	void __vectorcall EvalImpulse(
		const Vector3&    pos1, const Vector3&         pos2, const Vector3& point,
		float             penetration, const Vector3&  normal, float        invm1,
		float             invm2, const Vector3&        rv1, const Vector3&  rv2,
		const Vector3&    vel1, const Vector3&         vel2,
		const XMFLOAT3X3& inertiaT1, const XMFLOAT3X3& inertiaT2,
		Vector3&          linear1, Vector3&            linear2, Vector3& angular1,
		Vector3&          angular2, Vector3&           penetrationA,
		Vector3&          penetrationB
	);
}
