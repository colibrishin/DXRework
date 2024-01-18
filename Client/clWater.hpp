#pragma once
#include "egDelayedRenderObject.h"

namespace Client::Object
{
  class Water : public Objects::DelayedRenderObject
  {
  public:
    void Initialize() override;
    ~Water() override = default;

    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void OnDeserialized() override;
    void OnImGui() override;

  private:
    SERIALIZER_ACCESS
  };
} // namespace Client::Object

BOOST_CLASS_EXPORT_KEY(Client::Object::Water)
