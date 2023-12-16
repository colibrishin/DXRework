#pragma once

#include "clTriangleMesh.hpp"
#include <egText.h>

namespace Client::Object
{
    class FPSCounter : public Engine::Objects::Text
    {
    public:
        FPSCounter();
        void Initialize() override;
        ~FPSCounter() override;

        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;

    private:
        SERIALIZER_ACCESS
    };
} // namespace Client::Object

BOOST_CLASS_EXPORT_KEY(Client::Object::FPSCounter);
