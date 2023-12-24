#include "pch.h"
#include "clSkyBox.hpp"

#include "clBackSphereMesh.hpp"
#include "egModel.h"
#include "egModelRenderer.h"

SERIALIZER_ACCESS_IMPL(
                       Client::Object::SkyBox,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Object)))

namespace Client::Object
{
    inline SkyBox::SkyBox()
    : Object() {}

    inline void SkyBox::Initialize()
    {
        std::vector<StrongResource> resources;
        resources.push_back(Get<Resources::Mesh>("BackSphereMesh").lock());
        resources.push_back(Get<Resources::Texture>("Sky").lock());

        const auto model = Resources::Model::Create("Skybox", resources);

        const auto mr = AddComponent<Components::ModelRenderer>().lock();
        mr->SetModel(model);
        mr->AddVertexShader(Get<Graphic::VertexShader>("vs_default"));
        mr->AddPixelShader(Get<Graphic::PixelShader>("ps_default_nolight"));

        AddComponent<Components::Transform>();
        const auto tr = GetComponent<Components::Transform>().lock();
        tr->SetLocalPosition(Vector3(0.0f, 0.0f, 0.0f));
        tr->SetScale(Vector3::One * 15.0f);
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
