#include "pch.h"
#include "clTestCube.hpp"

#include <egCubeMesh.h>
#include <egNormalMap.h>

#include <egModel.h>

#include "egModelRenderer.h"
#include "egSoundPlayer.h"
#include "egSound.h"

SERIALIZER_ACCESS_IMPL(
                       Client::Object::TestCube,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Object)))

namespace Client::Object
{
    inline TestCube::TestCube()
    : Object() {}

    inline void TestCube::Initialize()
    {
        const auto model = Engine::Resources::Model::Get("CubeModel").lock();

        const auto mr = AddComponent<Engine::Components::ModelRenderer>().lock();
        mr->SetModel(model);
        mr->AddVertexShader(Graphic::VertexShader::Get("vs_default").lock());
        mr->AddPixelShader(Graphic::PixelShader::Get("ps_normalmap_specular"));

        AddComponent<Engine::Components::Transform>();
        AddComponent<Engine::Components::Collider>();
        const auto cldr = GetComponent<Engine::Components::Collider>().lock();
        cldr->SetType(Engine::BOUNDING_TYPE_BOX);
        cldr->SetMass(1.0f);

        AddComponent<Engine::Components::Rigidbody>();
        const auto rb = GetComponent<Engine::Components::Rigidbody>().lock();

        rb->SetFrictionCoefficient(0.1f);
        rb->SetGravityOverride(true);

        const auto snd = AddComponent<Engine::Components::SoundPlayer>().lock();
        snd->SetSound(Engine::GetResourceManager().GetResource<Engine::Resources::Sound>("AmbientSound").lock());
        snd->PlaySound();
    }

    inline TestCube::~TestCube() {}

    inline void TestCube::PreUpdate(const float& dt)
    {
        Object::PreUpdate(dt);
    }

    inline void TestCube::Update(const float& dt)
    {
        Object::Update(dt);
    }

    inline void TestCube::PreRender(const float& dt)
    {
        Object::PreRender(dt);
    }

    inline void TestCube::Render(const float& dt)
    {
        Object::Render(dt);
    }

    void TestCube::PostRender(const float& dt)
    {
        Object::PostRender(dt);
    }

    inline void TestCube::FixedUpdate(const float& dt)
    {
        Object::FixedUpdate(dt);
    }
} // namespace Client::Object
