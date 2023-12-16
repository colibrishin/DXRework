#pragma once
#include <egMesh.h>

namespace Client::Mesh
{
    class PlayerMesh : public Engine::Resources::Mesh
    {
    public:
        PlayerMesh();
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PreRender(const float& dt) override;

    protected:
        void Load_CUSTOM() override;
    };
}