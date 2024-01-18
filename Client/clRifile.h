#pragma once
#include <egObject.hpp>

namespace Client::Object
{
  class Rifle : public Abstract::Object
  {
  public:
    void Initialize() override;
    ~Rifle() override = default;

    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void FixedUpdate(const float& dt) override;

  private:
    SERIALIZER_ACCESS
  };
}

BOOST_CLASS_EXPORT_KEY(Client::Object::Rifle)
