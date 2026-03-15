#include "MeshModule.h"
#include "Mesh.h"

#include "Generator.hpp"


MODULE_IMPL(Engine::MeshModule, Mesh)

bool Engine::MeshModule::InitializeImpl()
{
#if WITH_EDITOR
    Resources::Mesh::Create("CubeMesh", CubeGenerator::GetCubeVerticesAsVector(), CubeGenerator::GetCubeIndicesAsVector());
    Resources::Mesh::Create("SphereMesh", DefaultSphereGenerator::GetSphereVerticesAsVector(), DefaultSphereGenerator::GetSphereIndicesAsVector());
    Resources::Mesh::Create(
         "PointMesh", VertexCollection{ {
                 {0.f, 0.f, 0.f},
                 {1.f, 0.f, 0.f, 1.f},
                 {0.f, 0.f},
                 {0.f, 0.f, 0.f},
                 {0.f, 0.f, 0.f},
                 {0.f, 0.f, 0.f},
                 Graphics::VertexBoneElement()} }, IndexCollection{ 0 }
        );
#endif

    return true;
}

bool Engine::MeshModule::ShutdownImpl()
{
    return true;
}

bool Engine::MeshModule::DynamicLoadable()
{
    return true;
}
