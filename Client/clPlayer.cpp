#include "pch.h"
#include "clPlayer.h"

#include "clCharacterController.hpp"
#include "clRifile.h"
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
        const auto cldr = AddComponent<Components::Collider>().lock();
        const auto rb   = AddComponent<Components::Rigidbody>().lock();
        const auto atr = AddComponent<Components::Animator>().lock();
        AddComponent<Client::State::CharacterController>();

        tr->SetLocalRotation(Quaternion::CreateFromYawPitchRoll({XM_PI / 2, 0, 0.0f}));
        cldr->SetModel(model);
        cldr->SetType(Engine::BOUNDING_TYPE_BOX);
        cldr->SetMass(1.0f);

        rb->SetFrictionCoefficient(0.1f);
        rb->SetGravityOverride(true);

        atr->SetAnimation(model->GetAnimationCatalog().front());

        const auto rifle = GetScene().lock()->CreateGameObject<Rifle>(Engine::LAYER_DEFAULT);
        AddChild(rifle);

        const auto bb_map = model->GetBoneBoundingBoxes();

        for (const auto& [idx, box] : bb_map)
        {
            const auto child = GetScene().lock()->CreateGameObject<Object>(GetLayer()).lock();
            const auto ctr = child->AddComponent<Components::Transform>().lock();
            child->AddComponent<Components::Collider>();
            // todo: if child is not set before the transform, world matrix is created in local matrix.
            AddChild(child);
            ctr->SetSizeAbsolute(true);
            ctr->SetRotateAbsolute(false);
            ctr->SetLocalPosition(box.Center);
            ctr->SetLocalRotation(box.Orientation);
            ctr->SetLocalScale(Vector3(box.Extents) * 2.f);
            m_child_bones_[idx] = child->GetLocalID();
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

        const auto cl = GetComponent<Components::Collider>().lock();
        const auto mr = GetComponent<Components::ModelRenderer>().lock();
        const auto atr = GetComponent<Components::Animator>().lock();
        const auto model = mr->GetModel().lock();
        const auto mtl = mr->GetMaterial().lock();
        const auto anim = mtl->GetResource<Resources::BoneAnimation>(atr->GetAnimation()).lock();
        auto deform = anim->GetFrameAnimation(atr->GetFrame());
        const auto rb = GetComponent<Components::Rigidbody>().lock();
        Vector3 min = {FLT_MAX, FLT_MAX, FLT_MAX};
        Vector3 max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

        for (const auto& [idx, id] : m_child_bones_)
        {
            const auto child = GetChild(id).lock();

            const auto ctr = child->GetComponent<Components::Transform>().lock();
            //ctr->SetAnimationMatrix(deform[idx].transform);

            static const std::vector<Vector3> stock_vertices
            {
                {0.5f, 0.5f, 0.5f},
                {0.5f, -0.5f, 0.5f},
                {-0.5f, -0.5f, 0.5f},
                {-0.5f, 0.5f, 0.5f},
                {0.5f, 0.5f, -0.5f},
                {0.5f, -0.5f, -0.5f},
                {-0.5f, -0.5f, -0.5f},
                {-0.5f, 0.5f, -0.5f},
            };

            std::vector<Vector3> out_vertices;
            out_vertices.resize(stock_vertices.size());

            XMVector3TransformCoordStream(
                out_vertices.data(), 
                sizeof(Vector3), 
                stock_vertices.data(), 
                sizeof(Vector3), 
                stock_vertices.size(), 
                ctr->GetLocalMatrix());

            for (const auto& v : out_vertices)
            {
                min = Vector3::Min(min, v);
                max = Vector3::Max(max, v);
            }
        }

        BoundingOrientedBox new_obb;
        BoundingBox         bb;
        BoundingBox::CreateFromPoints(bb, min, max);
        BoundingOrientedBox::CreateFromBoundingBox(new_obb, bb);
        cl->SetBoundingBox(new_obb);
    }
}
