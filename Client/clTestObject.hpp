#pragma once

#include "clTriangleMesh.hpp"
#include <egObject.hpp>

namespace Client::Object
{
    class TestObject : public Engine::Abstract::Object
    {
    public:
        TestObject()
        : Object() {}

        void Initialize() override;
        ~TestObject() override = default;

        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;

    private:
        SERIALIZER_ACCESS
    };
} // namespace Client::Object

BOOST_CLASS_EXPORT_KEY(Client::Object::TestObject)
