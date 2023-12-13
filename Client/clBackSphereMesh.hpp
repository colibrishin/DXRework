#pragma once
#include "../Engine/egMesh.hpp"

#include "GeometricPrimitive.h"

namespace Client::Mesh
{
	class BackSphereMesh final : public Engine::Resources::Mesh
	{
	public:
		BackSphereMesh();
		~BackSphereMesh() override = default;

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Load_CUSTOM() override;
		void Initialize() override;
		void FixedUpdate(const float& dt) override;

	private:
		SERIALIZER_ACCESS

	};
}

BOOST_CLASS_EXPORT_KEY(Client::Mesh::BackSphereMesh)