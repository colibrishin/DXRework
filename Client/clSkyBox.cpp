#include "pch.h"
#include "clSkyBox.hpp"

#include "egHelper.hpp"
#include "egMaterial.h"
#include "egModelRenderer.h"
#include "egShape.h"
#include "egTexture.h"
#include "egTransform.h"

SERIALIZER_ACCESS_IMPL(
                       Client::Object::SkyBox,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Object)))

namespace Client::Object
{
    inline SkyBox::SkyBox()
    : Object() {}

    inline void SkyBox::Initialize()
    {
        SetName("SkyBox");
        const auto mr = AddComponent<Components::ModelRenderer>().lock();
        mr->SetShape(Resources::Shape::Get("SphereModel"));
        mr->SetMaterial(Resources::Material::Get("SkyboxMaterial"));

        AddComponent<Components::Transform>();
        const auto tr = GetComponent<Components::Transform>().lock();
        tr->SetLocalPosition(Vector3(0.0f, 0.0f, 0.0f));
        tr->SetLocalScale(Vector3::One * 15.0f);
    }

    inline SkyBox::~SkyBox() {}

    inline void SkyBox::PreUpdate(const float& dt)
    {
        Object::PreUpdate(dt);
    }

    inline void SkyBox::Update(const float& dt)
    {
        Object::Update(dt);
    }

    void SkyBox::PostRender(const float& dt)
    {
        Object::PostRender(dt);
    }

    inline void SkyBox::PreRender(const float& dt)
    {
        Object::PreRender(dt);
    }

    inline void SkyBox::Render(const float& dt)
    {
        Object::Render(dt);
    }
} // namespace Client::Object
