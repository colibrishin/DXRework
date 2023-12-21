#include "pch.h"
#include "clPlayer.h"

#include "clCharacterController.hpp"
#include "clPlayerModel.h"
#include "egModelRenderer.h"

SERIALIZER_ACCESS_IMPL(
                       Client::Object::Player,
                       _ARTAG(_BSTSUPER(Object)))

namespace Client::Object
{
    void Player::Initialize()
    {
        Object::Initialize();

        const auto model = Resources::Model::Get("PlayerModel").lock();

        const auto mr = AddComponent<Components::ModelRenderer>().lock();

        mr->SetModel(model);
        mr->AddVertexShader(Graphic::VertexShader::Get("vs_default"));
        mr->AddPixelShader(Graphic::PixelShader::Get("ps_color"));

        AddComponent<Components::Transform>();
        const auto cldr = AddComponent<Components::Collider>().lock();
        const auto rb   = AddComponent<Components::Rigidbody>().lock();
        AddComponent<Client::State::CharacterController>();

        cldr->SetModel(model);
        cldr->SetBoundingBox(model->GetBoundingBox());
        cldr->SetType(Engine::BOUNDING_TYPE_BOX);
        cldr->SetDirtyWithTransform(true);
        cldr->SetMass(1.0f);

        const auto head_cldr = AddComponent<Components::Collider>().lock();
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
