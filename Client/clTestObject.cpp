#include "pch.h"
#include "clTestObject.hpp"

#include <egNormalMap.h>
#include <egSphereMesh.h>

#include "egModel.h"
#include "egModelRenderer.h"
#include "egShader.hpp"

SERIALIZER_ACCESS_IMPL(
                       Client::Object::TestObject,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Object)))

namespace Client::Object
{
    inline void TestObject::Initialize()
    {
        const auto model = Resources::Model::Get("SphereModel");
        const auto mr = AddComponent<Components::ModelRenderer>().lock();

        mr->SetModel(model);
        mr->AddVertexShader(Engine::Graphic::VertexShader::Get("vs_default"));
        mr->AddPixelShader(Engine::Graphic::PixelShader::Get("ps_normalmap"));

        AddComponent<Components::Transform>();
        AddComponent<Components::Collider>();
        const auto cldr = GetComponent<Components::Collider>().lock();
        cldr->SetType(BOUNDING_TYPE_SPHERE);
        cldr->SetDirtyWithTransform(true);
        cldr->SetMass(1.0f);

        AddComponent<Components::Rigidbody>();
        const auto rb = GetComponent<Components::Rigidbody>().lock();
        rb->SetFrictionCoefficient(0.1f);
        rb->SetGravityOverride(true);
    }

    inline void TestObject::PreUpdate(const float& dt)
    {
        Object::PreUpdate(dt);
    }

    inline void TestObject::Update(const float& dt)
    {
        Object::Update(dt);
    }

    inline void TestObject::PreRender(const float& dt)
    {
        Object::PreRender(dt);
    }

    inline void TestObject::Render(const float& dt)
    {
        Object::Render(dt);
    }

    void TestObject::PostRender(const float& dt)
    {
        Object::PostRender(dt);
    }
} // namespace Client::Object
