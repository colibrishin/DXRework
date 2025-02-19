#include "MeshModule.h"
#include "MeshModule.generated.h"
#include "Mesh.h"

#include "Components/Collider/Public/Generator.hpp"


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

const std::vector<std::string>& Engine::MeshModule::LoadAfter() const
{
    static std::vector<std::string> load_after = { "RenderPipeline" };
    return load_after;
}
