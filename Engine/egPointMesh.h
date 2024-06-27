#pragma once
#include "egMesh.h"

namespace Engine::Meshes
{
  class PointMesh final : public Resources::Mesh
  {
  public:
    PointMesh();
    ~PointMesh() override = default;

    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void Load_CUSTOM() override;
    void Initialize() override;
    void FixedUpdate(const float& dt) override;

  private:
    SERIALIZE_DECL
  };
} // namespace Engine::Mesh

BOOST_CLASS_EXPORT_KEY(Engine::Meshes::PointMesh)
