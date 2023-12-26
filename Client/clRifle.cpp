#include "pch.h"

#include "clCharacterController.hpp"
#include "clRifile.h"
#include "egCollider.hpp"
#include "egModel.h"
#include "egModelRenderer.h"
#include "egRigidbody.h"
#include "egShader.hpp"
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

        const auto model = Resources::Model::Get("RifleModel").lock();

        const auto mr = AddComponent<Components::ModelRenderer>().lock();

        mr->SetModel(model);
        mr->AddVertexShader(Graphic::VertexShader::Get("vs_default"));
        mr->AddPixelShader(Graphic::PixelShader::Get("ps_color"));

        const auto tr = AddComponent<Components::Transform>().lock();
        const auto cldr = AddComponent<Components::Collider>().lock();
        {
            // Shooting offset
            tr->SetLocalPosition({-.1f, 1.5f, 0.f});
            tr->SetYawPitchRoll({0.f, 90.f, 0.f});
            cldr->SetModel(model);
            cldr->SetOffsetRotation(tr->GetLocalRotation());
        }
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
