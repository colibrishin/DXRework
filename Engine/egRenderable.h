#pragma once
#include "egCommon.hpp"
#include "egEntity.hpp"

namespace Engine::Abstract
{
  class Renderable : public Entity
  {
  public:
    // Prepare for rendering.
    virtual void PreRender(const float& dt) = 0;
    // Run shader setup, texture and draw call.
    virtual void Render(const float& dt) = 0;
    // Run before the present call.
    virtual void PostRender(const float& dt) = 0;

  protected:
    Renderable() = default;

  private:
    SERIALIZE_DECL
  };
} // namespace Engine::Abstract

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Engine::Abstract::Renderable)
BOOST_CLASS_EXPORT_KEY(Engine::Abstract::Renderable)
