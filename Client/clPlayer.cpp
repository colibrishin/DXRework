#include "pch.h"
#include "clPlayer.h"

#include "clCharacterController.hpp"
#include "clPlayerMesh.h"
#include "clTestMesh.h"
#include "egMeshRenderer.h"

namespace Client::Object
{
    void Player::Initialize()
    {
        Object::Initialize();

        const auto mesh = Engine::GetResourceManager()
                          .GetResource<Mesh::TestMesh>("TestMesh").lock();

        const auto mr = AddComponent<Engine::Components::MeshRenderer>().lock();
        mr->SetMesh(mesh);
        mr->AddVertexShader(
                            Engine::GetResourceManager()
                            .GetResource<Engine::Graphic::VertexShader>("vs_default").lock());
        mr->AddPixelShader(
                           Engine::GetResourceManager()
                           .GetResource<Engine::Graphic::PixelShader>("ps_color").lock());


        AddComponent<Engine::Components::Transform>();
        const auto cldr = AddComponent<Engine::Components::Collider>().lock();
        const auto rb   = AddComponent<Engine::Components::Rigidbody>().lock();
        AddComponent<Client::State::CharacterController>();

        cldr->SetMesh(mesh);
        cldr->SetType(Engine::BOUNDING_TYPE_BOX);
        cldr->SetDirtyWithTransform(true);
        cldr->SetMass(1.0f);

        const auto head_cldr = AddComponent<Engine::Components::Collider>().lock();
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
