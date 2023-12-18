#include "pch.h"
#include "clTestObject.hpp"

#include <egNormalMap.h>
#include <egSphereMesh.h>

#include "egMeshRenderer.h"

SERIALIZER_ACCESS_IMPL(
                       Client::Object::TestObject,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Object)))

namespace Client::Object
{
    inline void TestObject::Initialize()
    {
        const auto mr = AddComponent<Engine::Components::MeshRenderer>().lock();
        mr->SetMesh(Engine::GetResourceManager()
                    .GetResource<Engine::Mesh::SphereMesh>("SphereMesh"));
        mr->Add(
                Engine::GetResourceManager()
                .GetResource<Engine::Resources::Texture>("TestTexture"));
        mr->Add(
                Engine::GetResourceManager()
                .GetResource<Engine::Graphic::VertexShader>("vs_default"));
        mr->Add(
                Engine::GetResourceManager()
                .GetResource<Engine::Graphic::PixelShader>("ps_normalmap"));
        mr->Add(
                Engine::GetResourceManager()
                .GetResource<Engine::Resources::NormalMap>("TestNormalMap"));

        AddComponent<Engine::Components::Transform>();
        AddComponent<Engine::Components::Collider>();
        const auto cldr = GetComponent<Engine::Components::Collider>().lock();
        cldr->SetType(Engine::BOUNDING_TYPE_SPHERE);
        cldr->SetDirtyWithTransform(true);
        cldr->SetMass(1.0f);

        AddComponent<Engine::Components::Rigidbody>();
        const auto rb = GetComponent<Engine::Components::Rigidbody>().lock();
        rb->SetFrictionCoefficient(0.1f);
        rb->SetGravityOverride(true);
    }

    inline void TestObject::PreUpdate(const float& dt)
    {
        Object::PreUpdate(dt);
    }

    inline void TestObject::Update(const float& dt)
    {
        Object::Update(dt);
    }

    inline void TestObject::PreRender(const float& dt)
    {
        Object::PreRender(dt);
    }

    inline void TestObject::Render(const float& dt)
    {
        Object::Render(dt);
    }

    void TestObject::PostRender(const float& dt)
    {
        Object::PostRender(dt);
    }
} // namespace Client::Object
