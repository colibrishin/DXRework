#pragma once
#include "egObject.hpp"

namespace Client::Object
{
  class Missile : public Engine::Abstract::Object
  {
  public:
    void Initialize() override;
    void PostRender(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void PreRender(const float& dt) override;
    void PreUpdate(const float& dt) override;
    void Render(const float& dt) override;
    void Update(const float& dt) override;

  private:
    SERIALIZER_ACCESS

  };
} // namespace Client::Object

BOOST_CLASS_EXPORT_KEY(Client::Object::Missile)