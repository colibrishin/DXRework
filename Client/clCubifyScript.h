#pragma once
#include <egScript.h>

#include "Client.h"

namespace Client::Scripts
{
  class CubifyScript : public Script
  {
  public:
	  CLIENT_SCRIPT_T(CubifyScript, SCRIPT_T_CUBIFY)

    explicit CubifyScript(const WeakObjectBase& owner);

	  ~CubifyScript() override;

    void Initialize() override;
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;

    void OnImGui() override;

    void SetCubeDimension(const Vector3& dimension);
    void SetCubeType(const eCubeType type);

    [[nodiscard]] WeakObjectBase GetDepthNearestCube(const Vector3& pos) const;
    [[nodiscard]] eCubeType GetCubeType() const;

  protected:
    void OnCollisionEnter(const WeakCollider& other) override;
    void OnCollisionContinue(const WeakCollider& other) override;
    void OnCollisionExit(const WeakCollider& other) override;

  private:
    SERIALIZE_DECL
    SCRIPT_CLONE_DECL

    CubifyScript();

    void UpdateCubes();

    std::vector<LocalActorID> m_cube_ids_;
    Vector3 m_cube_dimension_;

    eCubeType m_cube_type_;

    int m_z_length_;
    int m_x_length_;

  };
}

BOOST_CLASS_EXPORT_KEY(Client::Scripts::CubifyScript)