#pragma once
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

namespace Engine::Physics
{
	namespace GJK
	{
		bool __vectorcall GJKAlgorithm(
			const Matrix&           lhs_world,
			const Matrix&           rhs_world,
			const VertexCollection& lhs_vertices,
			const VertexCollection& rhs_vertices,
			const Vector3&          dir,
			Vector3&                normal,
			float&                  penetration,
			const std::size_t       max_gjk_iteration = 64,
			const std::size_t       max_epa_iteration = 64
		);
	} // namespace GJK
}
