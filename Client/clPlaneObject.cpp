#include "pch.h"
#include "clPlaneObject.hpp"

#include "egCubeMesh.hpp"
#include "egObject.hpp"
#include "egSound.hpp"
#include "egVertexShaderInternal.hpp"

CLIENT_OBJECT_IMPL(Client::Object::PlaneObject)

namespace Client::Object
{
    PlaneObject::PlaneObject() {}

    void PlaneObject::Initialize()
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
                    .GetResource<Engine::Graphic::PixelShader>("ps_color")
                    .lock());
        AddResource(
                    Engine::GetResourceManager()
                    .GetResource<Engine::Resources::Sound>("AmbientSound")
                    .lock());

        AddComponent<Engine::Component::Transform>();
        const auto tr = GetComponent<Engine::Component::Transform>().lock();
        tr->SetPosition(Vector3(0.0f, -1.0f, 0.0f));
        tr->SetScale({10.0f, 1.0f, 10.0f});

        AddComponent<Engine::Component::Collider>();
        const auto cldr = GetComponent<Engine::Component::Collider>().lock();
        cldr->SetType(Engine::BOUNDING_TYPE_BOX);
        cldr->SetDirtyWithTransform(true);
        cldr->SetMass(100000.0f);

        AddComponent<Engine::Component::Rigidbody>();
        const auto rb = GetComponent<Engine::Component::Rigidbody>().lock();

        rb->SetFixed(true);
        rb->SetFrictionCoefficient(0.2f);
        rb->SetGravityOverride(false);

        const auto test = GetResource<Engine::Resources::Sound>("AmbientSound");
        test.lock()->PlayLoop(GetSharedPtr<Object>());
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
