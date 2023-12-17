#include "pch.h"
#include "clTestMesh.h"

SERIALIZER_ACCESS_IMPL(Client::Mesh::TestMesh, _ARTAG(_BSTSUPER(Mesh)))

namespace Client::Mesh
{
	TestMesh::TestMesh() : Engine::Resources::Mesh("./bob_lamp_update.fbx")
    {
    }
}