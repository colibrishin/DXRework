#include "MeshModule.h"
#include "MeshModule.generated.h"
#include "Mesh.h"

#include "Components/Collider/Public/Generator.hpp"
#include "ModuleManager/Public/ModuleManager.h"

MODULE_IMPL(Engine::MeshModule, Mesh)

void Engine::MeshModule::Initialize()
{
    Resources::Mesh::Create("CubeMesh", CubeGenerator::GetCubeVerticesAsVector(), CubeGenerator::GetCubeIndicesAsVector());
    Resources::Mesh::Create("SphereMesh", DefaultSphereGenerator::GetSphereVerticesAsVector(), DefaultSphereGenerator::GetSphereIndicesAsVector());
}

void Engine::MeshModule::Shutdown()
{
    
}

bool Engine::MeshModule::DynamicLoadable()
{
    return true;
}
