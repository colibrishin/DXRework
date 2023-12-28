#include "pch.h"

#include "clCharacterController.hpp"
#include "clRifile.h"
#include "egAnimator.h"
#include "egBaseAnimation.h"
#include "egCollider.hpp"
#include "egMaterial.h"
#include "egModelRenderer.h"
#include "egRigidbody.h"
#include "egShader.hpp"
#include "egShape.h"
#include "egTransform.h"
#include "egVertexShaderInternal.h"

SERIALIZER_ACCESS_IMPL(
                       Client::Object::Rifle,
                       _ARTAG(_BSTSUPER(Object)))

namespace Client::Object
{
    void Rifle::Initialize()
    {
        Object::Initialize();

        const auto model = Resources::Shape::Get("RifleModel").lock();

        const auto mr = AddComponent<Components::ModelRenderer>().lock();

        mr->SetShape(model);
        mr->SetMaterial(Resources::Material::Get("RifleColorMaterial"));

        const auto tr = AddComponent<Components::Transform>().lock();
        const auto atr = AddComponent<Components::Animator>().lock();
        const auto cldr = AddComponent<Components::Collider>().lock();

        atr->SetAnimation("FireAnimation");
        cldr->SetModel(model);
    }

    void Rifle::PreUpdate(const float& dt)
    {
        Object::PreUpdate(dt);
    }

    void Rifle::Update(const float& dt)
    {
        Object::Update(dt);
    }

    void Rifle::PreRender(const float& dt)
    {
        Object::PreRender(dt);
    }

    void Rifle::Render(const float& dt)
    {
        Object::Render(dt);
    }

    void Rifle::PostRender(const float& dt)
    {
        Object::PostRender(dt);
    }

    void Rifle::FixedUpdate(const float& dt)
    {
        Object::FixedUpdate(dt);
    }
}
