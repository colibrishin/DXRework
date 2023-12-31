#pragma once
#include <egMesh.h>

#include "GeometricPrimitive.h"

namespace Client::Meshes
{
    class BackSphereMesh final : public Engine::Resources::Mesh
    {
    public:
        BackSphereMesh();
        ~BackSphereMesh() override = default;

        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void Load_CUSTOM() override;
        void Initialize() override;
        void FixedUpdate(const float& dt) override;

    private:
        SERIALIZER_ACCESS
    };
} // namespace Client::Mesh

BOOST_CLASS_EXPORT_KEY(Client::Meshes::BackSphereMesh)
