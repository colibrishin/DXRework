#include "pch.h"
#include "clTestObject.hpp"

#include <egNormalMap.h>
#include <egSphereMesh.h>

SERIALIZER_ACCESS_IMPL(
                       Client::Object::TestObject,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Object)))

namespace Client::Object
{
    inline void TestObject::Initialize()
    {
        AddResource(
                    Engine::GetResourceManager()
                    .GetResource<Engine::Mesh::SphereMesh>("SphereMesh")
                    .lock());
        AddResource(
                    Engine::GetResourceManager()
                    .GetResource<Engine::Resources::Texture>("TestTexture")
                    .lock());
        AddResource(
                    Engine::GetResourceManager()
                    .GetResource<Engine::Resources::NormalMap>("TestNormalMap")
                    .lock());
        AddResource(
                    Engine::GetResourceManager()
                    .GetResource<Engine::Graphic::VertexShader>("vs_default")
                    .lock());
        AddResource(
                    Engine::GetResourceManager()
                    .GetResource<Engine::Graphic::PixelShader>("ps_normalmap")
                    .lock());

        AddComponent<Engine::Component::Transform>();
        AddComponent<Engine::Component::Collider>();
        const auto cldr = GetComponent<Engine::Component::Collider>().lock();
        cldr->SetType(Engine::BOUNDING_TYPE_SPHERE);
        cldr->SetDirtyWithTransform(true);
        cldr->SetMass(1.0f);

        AddComponent<Engine::Component::Rigidbody>();
        const auto rb = GetComponent<Engine::Component::Rigidbody>().lock();
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
