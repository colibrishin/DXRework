#pragma once
#include "Mesh.h"
#include "CubeMesh.generated.h"

namespace Engine::Meshes
{
	ECLASS(resource, serialize)
	class CubeMesh : public Resources::Mesh
	{
		GENERATE_BODY
	public:
		CubeMesh();
		~CubeMesh() override = default;

		void Load_CUSTOM() override;
		void Initialize() override;
	};
} // namespace Engine::Mesh
