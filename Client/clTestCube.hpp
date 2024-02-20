#pragma once

#include <egObject.hpp>
#include "clTriangleMesh.hpp"

namespace Client::Object
{
  class TestCube final : public Abstract::Object
  {
  public:
    TestCube();
    void Initialize() override;
    ~TestCube() override;

    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void FixedUpdate(const float& dt) override;

  private:
    SERIALIZER_ACCESS
  };
} // namespace Client::Object

BOOST_CLASS_EXPORT_KEY(Client::Object::TestCube)
