#include "pch.h"
#include "clSkyBox.hpp"

#include "clBackSphereMesh.hpp"

SERIALIZER_ACCESS_IMPL(
                       Client::Object::SkyBox,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Object)))

namespace Client::Object
{
    inline SkyBox::SkyBox()
    : Object() {}

    inline void SkyBox::Initialize()
    {
        AddResource(
                    Engine::GetResourceManager()
                    .GetResource<Mesh::BackSphereMesh>("BackSphereMesh")
                    .lock());
        AddResource(
                    Engine::GetResourceManager()
                    .GetResource<Engine::Resources::Texture>("Sky")
                    .lock());
        AddResource(
                    Engine::GetResourceManager()
                    .GetResource<Engine::Graphic::VertexShader>("vs_default")
                    .lock());
        AddResource(
                    Engine::GetResourceManager()
                    .GetResource<Engine::Graphic::PixelShader>("ps_default_nolight")
                    .lock());

        AddComponent<Engine::Components::Transform>();
        const auto tr = GetComponent<Engine::Components::Transform>().lock();
        tr->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
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
