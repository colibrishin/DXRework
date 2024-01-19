#pragma once
#include "egObject.hpp"

namespace Client::Objects
{
  class Hitbox : public Abstract::Object
  {
  public:
    Hitbox() = default;
    ~Hitbox() override = default;

    void Initialize() override;
    void SetBoundingBox(const BoundingOrientedBox& box);

  private:
    SERIALIZER_ACCESS
    BoundingOrientedBox m_bounding_box_;

    void OnCollisionContinue(const StrongCollider& other) override;
    void OnCollisionEnter(const StrongCollider& other) override;
    void OnCollisionExit(const StrongCollider& other) override;

  };
}

BOOST_CLASS_EXPORT_KEY(Client::Objects::Hitbox);