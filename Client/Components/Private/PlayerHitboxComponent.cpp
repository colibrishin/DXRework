#include "Components/Public/PlayerHitboxComponent.h"
#include "PlayerHitboxComponent.generated.h"

#include "ObjectBase/Public/ObjectBase.h"
#include "Shape.h"
#include "ModelRenderer.h"
#include "Animator.h"
#include "BoneAnimation.h"
#include "Components/Collider/Public/Collider.h"
#include "Components/Transform/Public/Transform.h"
#include "Components/Rigidbody/Public/Rigidbody.h"
#include <Objects/Object/Public/Object.h>

void PlayerHitboxComponent::Initialize()
{
    Component::Initialize();

    const auto& model = Engine::Resources::Shape::Get("CharacterShape");

    if (const auto& locked = model.lock())
    {
        const auto& bb_map = locked->GetBoneBoundingBoxes();
        const auto& obj = GetOwner().lock();

        for (const auto& [idx, box] : bb_map)
        {
            const auto& child = obj->GetScene().lock()->CreateGameObject<Engine::Object>(
                Engine::RESERVED_LAYER_DEFAULT).lock();
            const auto& ctr = child->AddComponent<Engine::Components::Transform>().lock();
            child->AddComponent<Engine::Components::Collider>();

            ctr->SetLocalPosition(box.Center);
            ctr->SetLocalScale(Vector3(box.Extents) * 2.f);
            ctr->SetLocalRotation(box.Orientation);

            child->SetName("Bone" + std::to_string(idx));
            obj->AddChild(child);
        }
    }
}

void PlayerHitboxComponent::PreUpdate(const float dt) {}

void PlayerHitboxComponent::PostUpdate(const float dt)
{
}

void PlayerHitboxComponent::Update(const float dt) {}

void PlayerHitboxComponent::FixedUpdate(const float dt)
{
    updateHitBox();
}

void PlayerHitboxComponent::OnSerialized() {}

void PlayerHitboxComponent::OnDeserialized() {}

Engine::Weak<Engine::Abstracts::ObjectBase> PlayerHitboxComponent::GetHead() const 
{
    if (const auto& owner = GetOwner().lock())
    {
        for (const auto& child : owner->GetChildren())
        {
            if (const auto& locked = child.lock())
            {
                if (locked->GetName().find("Bone5") != std::string::npos)
                {
                    return locked;
                }
            }
        }
    }

    return {};
}


void PlayerHitboxComponent::updateHitBox() const 
{
    const auto obj = GetOwner().lock();
    const auto cl  = obj->GetComponent<Engine::Components::Collider>().lock();
    const auto mr  = obj->GetComponent<Engine::Components::ModelRenderer>().lock();
    const auto atr = obj->GetComponent<Engine::Components::Animator>().lock();

    if (!cl || !mr || !atr)
    {
        return;
    }

    const auto& shape = mr->GetShape().lock();

    if (!shape)
    {
        return;
    }

    const auto& anim_tex = shape->GetAnimations().lock();

    if (!anim_tex)
    {
        return;
    }

    if (const auto anim = anim_tex->GetAnimation(atr->GetAnimation()).lock())
    {
        const auto deform = anim->GetFrameAnimation(atr->GetFrame());
        const auto rb     = obj->GetComponent<Engine::Components::Rigidbody>().lock();
        Vector3    min    = {FLT_MAX, FLT_MAX, FLT_MAX};
        Vector3    max    = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

        for (const auto& child : GetOwner().lock()->GetChildren())
        {
            if (const auto locked = child.lock();
                locked)
            {
                const auto& name = locked->GetName();
                const auto  pos  = name.find("Bone");

                if (pos == std::string::npos)
                {
                    continue;
                }

                const auto ctr = locked->GetComponent<Engine::Components::Transform>().lock();
                const auto idx = std::stoi(name.substr(4));

                ctr->SetAnimationMatrix(deform[idx]);

                static constexpr Vector3 stock_vertices[]
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
                out_vertices.resize(std::size(stock_vertices));

                XMVector3TransformCoordStream
                        (
                            out_vertices.data(),
                            sizeof(Vector3),
                            stock_vertices,
                            sizeof(Vector3),
                            std::size(stock_vertices),
                            ctr->GetLocalMatrix()
                        );

                for (const auto& v : out_vertices)
                {
                    min = Vector3::Min(min, v);
                    max = Vector3::Max(max, v);
                }
            }
        }

        Engine::BoundingOrientedBox new_obb;
        Engine::BoundingBox         bb;
        Engine::BoundingBox::CreateFromPoints(bb, min, max);
        Engine::BoundingOrientedBox::CreateFromBoundingBox(new_obb, bb);
        cl->SetBoundingBox(new_obb);
    }
}

void PlayerHitboxComponent::onCollisionEnter(const Engine::Weak<Engine::Components::Collider>& other)
{
}

void PlayerHitboxComponent::onCollisionContinue(const Engine::Weak<Engine::Components::Collider>& other)
{
}

void PlayerHitboxComponent::onCollisionExit(const Engine::Weak<Engine::Components::Collider>& other)
{
}

Engine::eComponentUpdatePriorities PlayerHitboxComponent::GetUpdatePriority() const
{
    return Engine::eComponentUpdatePriority::COM_PRIORITY_PHYSICS;
}
