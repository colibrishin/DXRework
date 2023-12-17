#pragma once
#include <egMesh.h>

namespace Client::Mesh
{
    class TestMesh : public Engine::Resources::Mesh
    {
    public:
        TestMesh();

    private:
        SERIALIZER_ACCESS
    };
} // namespace Client::Mesh

BOOST_CLASS_EXPORT_KEY(Client::Mesh::TestMesh)