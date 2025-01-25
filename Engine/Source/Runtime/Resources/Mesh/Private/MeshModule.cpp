#include "MeshModule.h"

#include "CubeMesh.h"
#include "MeshModule.generated.h"
#include "SphereMesh.h"

#include "ModuleManager/Public/ModuleManager.h"

MODULE_IMPL(Engine::MeshModule, Mesh)

void Engine::MeshModule::Initialize()
{
    Meshes::CubeMesh::Create("CubeMesh");
    Meshes::SphereMesh::Create("SphereMesh");
}

void Engine::MeshModule::Shutdown()
{
    
}

bool Engine::MeshModule::DynamicLoadable()
{
    return true;
}
