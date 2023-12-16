#include "pch.h"
#include "clTestCube.hpp"

#include <egCubeMesh.h>
#include <egNormalMap.h>

SERIALIZER_ACCESS_IMPL(
                       Client::Object::TestCube,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Object)))

namespace Client::Object
{
    inline TestCube::TestCube()
    : Object() {}

    inline void TestCube::Initialize()
    {
        AddResource(
                    Engine::GetResourceManager()
                    .GetResource<Engine::Mesh::CubeMesh>("CubeMesh")
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
                    .GetResource<Engine::Graphic::PixelShader>("ps_normalmap_specular")
                    .lock());

        AddComponent<Engine::Component::Transform>();
        AddComponent<Engine::Component::Collider>(
                                                  GetResource<Engine::Resources::Mesh>("CubeMesh"));
        const auto cldr = GetComponent<Engine::Component::Collider>().lock();
        cldr->SetType(Engine::BOUNDING_TYPE_BOX);
        cldr->SetDirtyWithTransform(true);
        cldr->SetMass(1.0f);

        AddComponent<Engine::Component::Rigidbody>();
        const auto rb = GetComponent<Engine::Component::Rigidbody>().lock();

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
