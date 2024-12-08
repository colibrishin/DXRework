#pragma once
#include <PxPhysics.h>

#include <cooking/PxCooking.h>

#include <extensions/PxDefaultStreams.h>

#include "egPhysicsManager.h"
#ifdef PHYSX_ENABLED

namespace Engine
{
	static void __vectorcall CookMesh(
		const VertexCollection& vertices, 
		const IndexCollection& indices, 
		physx::PxTriangleMesh** built_shape,
		physx::PxSDFDesc** built_sdf = nullptr
	)
	{
		physx::PxTriangleMeshDesc mesh_desc;
		mesh_desc.points.count  = static_cast<physx::PxU32>(vertices.size());
		mesh_desc.points.data   = vertices.data();
		mesh_desc.points.stride = sizeof(Graphics::VertexElement);

		mesh_desc.triangles.count  = static_cast<physx::PxU32>(indices.size() / 3);
		mesh_desc.triangles.stride = 3 * sizeof(UINT);
		mesh_desc.triangles.data   = indices.data();

		// todo: prebuild
		if (built_sdf)
		{
			(*built_sdf)->spacing = 0.125f;
			mesh_desc.sdfDesc = *built_sdf;
		}

		physx::PxCookingParams cooking_params(GetPhysicsManager().GetPhysX()->getTolerancesScale());
		cooking_params.buildGPUData = true;

		physx::PxTriangleMeshCookingResult::Enum result;
		physx::PxDefaultMemoryOutputStream       out_stream;

		if (PxCookTriangleMesh(cooking_params, mesh_desc, out_stream, &result))
		{
			physx::PxDefaultMemoryInputData input_steam
					(
					 out_stream.getData(),
					 out_stream.getSize()
					);

			(*built_shape) = GetPhysicsManager().GetPhysX()->createTriangleMesh(input_steam);

			// todo: serialize sdf information after cooking
		}
		else
		{
			OutputDebugStringW(L"Failed to cook as stock collider triangle mesh by physx!");
		}
	}

}
#endif 