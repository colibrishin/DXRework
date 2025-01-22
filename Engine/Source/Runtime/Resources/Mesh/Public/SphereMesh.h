#pragma once
#include "Mesh.h"
#include "SphereMesh.generated.h"

namespace Engine::Meshes
{
	ECLASS(resource, serialize)
	class SphereMesh : public Resources::Mesh
	{
		GENERATE_BODY
	public:
		SphereMesh();
		~SphereMesh() override = default;

		void Load_CUSTOM() override;
		void Initialize() override;
	};
} // namespace Engine::Mesh
