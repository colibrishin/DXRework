#include "pch.h"
#include "clPlayerMesh.h"

namespace Client::Mesh
{
    PlayerMesh::PlayerMesh() : Engine::Resources::Mesh("./player.fbx") {}

	void PlayerMesh::PreUpdate(const float& dt) {}

	void PlayerMesh::Update(const float& dt) {}

	void PlayerMesh::FixedUpdate(const float& dt) {}

	void PlayerMesh::PreRender(const float& dt) {}

	void PlayerMesh::Load_CUSTOM()
	{}
}
