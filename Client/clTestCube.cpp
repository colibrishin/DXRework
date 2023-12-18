#include "pch.h"
#include "clTestCube.hpp"

#include <egCubeMesh.h>
#include <egNormalMap.h>

#include "egMeshRenderer.h"

SERIALIZER_ACCESS_IMPL(
                       Client::Object::TestCube,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Object)))

namespace Client::Object
{
    inline TestCube::TestCube()
    : Object() {}

    inline void TestCube::Initialize()
    {
        const auto mr = AddComponent<Engine::Components::MeshRenderer>().lock();
        mr->SetMesh(Engine::GetResourceManager().GetResource<Engine::Mesh::CubeMesh>("CubeMesh"));
        mr->Add(Engine::GetResourceManager().GetResource<Engine::Resources::NormalMap>("TestNormalMap"));
        mr->Add(Engine::GetResourceManager().GetResource<Engine::Resources::Texture>("TestTexture"));
        mr->Add(Engine::GetResourceManager().GetResource<Engine::Graphic::VertexShader>("vs_default"));
        mr->Add(
                Engine::GetResourceManager().GetResource<Engine::Graphic::PixelShader>(
                     "ps_normalmap_specular"));

        AddComponent<Engine::Components::Transform>();
        AddComponent<Engine::Components::Collider>();
        const auto cldr = GetComponent<Engine::Components::Collider>().lock();
        cldr->SetType(Engine::BOUNDING_TYPE_BOX);
        cldr->SetDirtyWithTransform(true);
        cldr->SetMass(1.0f);

        AddComponent<Engine::Components::Rigidbody>();
        const auto rb = GetComponent<Engine::Components::Rigidbody>().lock();

        rb->SetFrictionCoefficient(0.1f);
        rb->SetGravityOverride(true);
    }

    inline TestCube::~TestCube() {}

    inline void TestCube::PreUpdate(const float& dt)
    {
        Object::PreUpdate(dt);
    }

    inline void TestCube::Update(const float& dt)
    {
        Object::Update(dt);
    }

    inline void TestCube::PreRender(const float& dt)
    {
        Object::PreRender(dt);
    }

    inline void TestCube::Render(const float& dt)
    {
        Object::Render(dt);
    }

    void TestCube::PostRender(const float& dt)
    {
        Object::PostRender(dt);
    }

    inline void TestCube::FixedUpdate(const float& dt)
    {
        Object::FixedUpdate(dt);
    }
} // namespace Client::Object
