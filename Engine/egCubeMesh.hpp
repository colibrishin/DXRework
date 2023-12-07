#pragma once
#include "egMesh.hpp"
#include "GeometricPrimitive.h"

namespace Engine::Mesh
{
	class CubeMesh final : public Resources::Mesh
	{
	public:
		CubeMesh();
		~CubeMesh() override = default;

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Load_CUSTOM() override;
		void Initialize() override;
		void FixedUpdate(const float& dt) override;

	private:
		SERIALIZER_ACCESS

	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Mesh::CubeMesh)