#pragma once
#include <egMesh.h>

namespace Client::Mesh
{
    class PlayerMesh final : public Engine::Resources::Mesh
    {
    public:
        PlayerMesh();
        ~PlayerMesh() override = default;

        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void Load_CUSTOM() override;
        void FixedUpdate(const float& dt) override;

    private:
        SERIALIZER_ACCESS
    };
} // namespace Client::Mesh

BOOST_CLASS_EXPORT_KEY(Client::Mesh::PlayerMesh)
