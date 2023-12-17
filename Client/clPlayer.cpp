#include "pch.h"
#include "clPlayer.h"

#include "clCharacterController.hpp"
#include "clPlayerMesh.h"
#include "clTestMesh.h"

namespace Client::Object
{
    void Player::Initialize()
    {
        Object::Initialize();

        const auto mesh = Engine::GetResourceManager()
                          .GetResource<Mesh::TestMesh>("TestMesh").lock();

        AddResource(mesh);

        AddResource(
                    Engine::GetResourceManager()
                    .GetResource<Engine::Graphic::VertexShader>("vs_default").lock());

        AddResource(
                    Engine::GetResourceManager()
                    .GetResource<Engine::Graphic::PixelShader>("ps_color").lock());

        AddComponent<Engine::Component::Transform>();
        const auto cldr = AddComponent<Engine::Component::Collider>().lock();
        const auto rb   = AddComponent<Engine::Component::Rigidbody>().lock();
        AddComponent<Client::State::CharacterController>();

        cldr->SetMesh(mesh);
        cldr->SetType(Engine::BOUNDING_TYPE_BOX);
        cldr->SetDirtyWithTransform(true);
        cldr->SetMass(1.0f);

        const auto head_cldr = AddComponent<Engine::Component::Collider>().lock();
        head_cldr->SetType(Engine::BOUNDING_TYPE_SPHERE);
        head_cldr->SetDirtyWithTransform(true);
        head_cldr->SetMass(1.0f);
        head_cldr->SetOffset({0.f, 2.5f, 0.f});

        rb->SetMainCollider(cldr);
        rb->SetFrictionCoefficient(0.1f);
        rb->SetGravityOverride(true);
    }

    void Player::PreUpdate(const float& dt)
    {
        Object::PreUpdate(dt);
    }

    void Player::Update(const float& dt)
    {
        Object::Update(dt);
    }

    void Player::PreRender(const float& dt)
    {
        Object::PreRender(dt);
    }

    void Player::Render(const float& dt)
    {
        Object::Render(dt);
    }

    void Player::PostRender(const float& dt)
    {
        Object::PostRender(dt);
    }

    void Player::FixedUpdate(const float& dt)
    {
        Object::FixedUpdate(dt);
    }
}
