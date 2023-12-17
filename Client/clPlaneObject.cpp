#include "pch.h"
#include "clPlaneObject.hpp"

#include <egCubeMesh.h>
#include <egObject.hpp>
#include <egSound.h>
#include <egVertexShaderInternal.h>
#include <egNormalMap.h>

#include "egMeshRenderer.h"

SERIALIZER_ACCESS_IMPL(
                       Client::Object::PlaneObject,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Object)))

namespace Client::Object
{
    PlaneObject::PlaneObject() {}

    void PlaneObject::Initialize()
    {
        const auto mr = AddComponent<Engine::Components::MeshRenderer>().lock();
        mr->SetMesh(Engine::GetResourceManager().GetResource<Engine::Mesh::CubeMesh>("CubeMesh").lock());
        mr->AddTexture(Engine::GetResourceManager().GetResource<Engine::Resources::Texture>("TestTexture").lock());
        mr->AddNormalMap(
                         Engine::GetResourceManager().GetResource<Engine::Resources::NormalMap>(
                          "TestNormalMap").lock());
        mr->AddVertexShader(
                            Engine::GetResourceManager().GetResource<Engine::Graphic::VertexShader>("vs_default").
                                                         lock());
        mr->AddPixelShader(Engine::GetResourceManager().GetResource<Engine::Graphic::PixelShader>("ps_color").lock());

        AddComponent<Engine::Components::Transform>();
        const auto tr = GetComponent<Engine::Components::Transform>().lock();
        tr->SetPosition(Vector3(0.0f, -1.0f, 0.0f));
        tr->SetScale({10.0f, 1.0f, 10.0f});

        AddComponent<Engine::Components::Collider>();
        const auto cldr = GetComponent<Engine::Components::Collider>().lock();
        cldr->SetType(Engine::BOUNDING_TYPE_BOX);
        cldr->SetDirtyWithTransform(true);
        cldr->SetMass(100000.0f);

        AddComponent<Engine::Components::Rigidbody>();
        const auto rb = GetComponent<Engine::Components::Rigidbody>().lock();

        rb->SetFixed(true);
        rb->SetFrictionCoefficient(0.2f);
        rb->SetGravityOverride(false);
    }

    PlaneObject::~PlaneObject() {}

    void PlaneObject::PreUpdate(const float& dt)
    {
        Object::PreUpdate(dt);
    }

    void PlaneObject::Update(const float& dt)
    {
        Object::Update(dt);
    }

    void PlaneObject::PreRender(const float& dt)
    {
        Object::PreRender(dt);
    }

    void PlaneObject::Render(const float& dt)
    {
        Object::Render(dt);
    }

    void PlaneObject::PostRender(const float& dt)
    {
        Object::PostRender(dt);
    }
} // namespace Client::Object
