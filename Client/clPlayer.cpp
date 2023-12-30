#include "pch.h"
#include "clPlayer.h"

#include "clCharacterController.hpp"
#include "egAnimator.h"
#include "egBaseCollider.hpp"
#include "egModelRenderer.h"
#include "egRigidbody.h"
#include "egShader.hpp"
#include "egTransform.h"
#include "egVertexShaderInternal.h"
#include "egBoneAnimation.h"
#include "egMaterial.h"
#include "egShape.h"

SERIALIZER_ACCESS_IMPL(
                       Client::Object::Player,
                       _ARTAG(_BSTSUPER(Object)))

namespace Client::Object
{
    void Player::Initialize()
    {
        Object::Initialize();

        const auto model = Resources::Shape::Get("CharacterModel").lock();

        const auto mr = AddComponent<Components::ModelRenderer>().lock();
        mr->SetShape(model);
        mr->SetMaterial(Resources::Material::Get("CharacterMaterial"));

        const auto tr = AddComponent<Components::Transform>().lock();
        const auto cldr = AddComponent<Components::BaseCollider>().lock();
        const auto rb   = AddComponent<Components::Rigidbody>().lock();
        const auto atr = AddComponent<Components::Animator>().lock();
        AddComponent<Client::State::CharacterController>();

        tr->SetLocalRotation(Quaternion::CreateFromYawPitchRoll({0, XM_PI / 2, 0.0f}));
        cldr->SetModel(model);
        cldr->SetType(Engine::BOUNDING_TYPE_BOX);
        cldr->SetMass(1.0f);

        rb->SetMainCollider(cldr);
        rb->SetFrictionCoefficient(0.1f);
        rb->SetGravityOverride(true);

        atr->SetAnimation(model->GetAnimationCatalog().front());

        for (const auto& [idx, box] : model->GetBoneBoundingBoxes())
        {
            const auto bone_cldr = AddComponent<Components::BaseCollider>().lock();
            bone_cldr->SetBoundingBox(box);
            m_bone_colliders_.emplace(idx, bone_cldr);
        }
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

        const auto tr = GetComponent<Components::Transform>().lock();
        const auto mr = GetComponent<Components::ModelRenderer>().lock();
        const auto atr = GetComponent<Components::Animator>().lock();
        const auto model = mr->GetModel().lock();
        const auto mtl = mr->GetMaterial().lock();
        const auto anim = mtl->GetResource<Resources::BoneAnimation>(atr->GetAnimation()).lock();
        auto deform = anim->GetFrameAnimation(atr->GetFrame());

        for (const auto& [idx, cldr] : m_bone_colliders_)
        {
            cldr->FromMatrix(deform[idx].transform);
        }
    }
}
