#pragma once

#include <egText.h>

namespace Client::Object
{
  class HealthText : public Objects::Text
  {
  public:
    HealthText();
    void Initialize() override;
    ~HealthText() override;

    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;

  private:
    SERIALIZER_ACCESS
  };
} // namespace Client::Object

BOOST_CLASS_EXPORT_KEY(Client::Object::HealthText);
