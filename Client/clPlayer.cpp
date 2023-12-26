#include "pch.h"
#include "clPlayer.h"

#include "clCharacterController.hpp"
#include "egAnimator.h"
#include "egCollider.hpp"
#include "egModel.h"
#include "egModelRenderer.h"
#include "egRigidbody.h"
#include "egShader.hpp"
#include "egTransform.h"
#include "egVertexShaderInternal.h"
#include "egBoneAnimation.h"

SERIALIZER_ACCESS_IMPL(
                       Client::Object::Player,
                       _ARTAG(_BSTSUPER(Object)))

namespace Client::Object
{
    void Player::Initialize()
    {
        Object::Initialize();

        const auto model = Resources::Model::Get("CharacterModel").lock();

        const auto mr = AddComponent<Components::ModelRenderer>().lock();

        mr->SetModel(model);
        mr->AddVertexShader(Graphic::VertexShader::Get("vs_default"));
        mr->AddPixelShader(Graphic::PixelShader::Get("ps_color"));

        const auto tr = AddComponent<Components::Transform>().lock();
        const auto cldr = AddComponent<Components::Collider>().lock();
        const auto rb   = AddComponent<Components::Rigidbody>().lock();
        const auto atr = AddComponent<Components::Animator>().lock();
        AddComponent<Client::State::CharacterController>();

        tr->SetLocalRotation(Quaternion::CreateFromYawPitchRoll({0, XM_PI / 2, 0.0f}));
        cldr->SetModel(model);
        cldr->SetBoundingBox(model->GetBoundingBox());
        cldr->SetType(Engine::BOUNDING_TYPE_BOX);
        cldr->SetMass(1.0f);

        rb->SetMainCollider(cldr);
        rb->SetFrictionCoefficient(0.1f);
        rb->SetGravityOverride(true);

        atr->SetAnimation(model->GetAnimation(0));
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
