#pragma once
#include "Mesh.h"
#include "SphereMesh.generated.h"

namespace Engine::Meshes
{
	ECLASS(serialize)
	class SphereMesh : public Resources::Mesh
	{
	public:
		SphereMesh();
		~SphereMesh() override = default;

		void Load_CUSTOM() override;
		void Initialize() override;
	};
} // namespace Engine::Mesh
