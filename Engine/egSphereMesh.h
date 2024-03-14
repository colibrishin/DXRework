#pragma once
#include "GeometricPrimitive.h"
#include "egMesh.h"

namespace Engine::Meshes
{
  class SphereMesh final : public Resources::Mesh
  {
  public:
    SphereMesh();
    ~SphereMesh() override = default;

    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PreRender(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void Load_CUSTOM() override;
    void Initialize() override;
    void FixedUpdate(const float& dt) override;

  private:
    SERIALIZE_DECL
  };
} // namespace Engine::Mesh

BOOST_CLASS_EXPORT_KEY(Engine::Meshes::SphereMesh)
