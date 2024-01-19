#pragma once
#include <egObject.hpp>

namespace Client::Object
{
  class Player : public Abstract::Object
  {
  public:
    void Initialize() override;
    ~Player() override = default;

    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void UpdateHitboxes(
    );
    void FixedUpdate(const float& dt) override;

    WeakObject GetHead() const;

  private:
    SERIALIZER_ACCESS
    std::map<UINT, LocalComponentID> m_child_bones_;
    WeakObject                       m_head_;
  };
}

BOOST_CLASS_EXPORT_KEY(Client::Object::Player)
