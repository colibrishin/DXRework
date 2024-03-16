#pragma once
#include "egCommon.hpp"

namespace Engine
{
  class Script : public Abstract::Renderable
  {
  public:
    explicit Script(eScriptType type, const WeakObjectBase& owner);

    virtual void       SetActive(bool active);
    bool       GetActive() const { return m_b_active_; }
    WeakObjectBase GetOwner() const { return m_owner_; }
    eScriptType GetScriptType() const { return m_type_; }

    [[nodiscard]] StrongScript Clone(const WeakObjectBase& owner) const;

    void OnSerialized() override;

    using ScriptFactoryFunction = std::function<StrongScript(const WeakObjectBase&)>;

    template <typename T, typename SLock = std::enable_if_t<std::is_base_of_v<Script, T>>>
    static void Register() { s_script_factory_[typeid(T).name()] = &T::Create; }

    static const std::map<std::string, ScriptFactoryFunction>& GetScriptFactory() { return s_script_factory_; }

  protected:
    inline static std::map<std::string, ScriptFactoryFunction> s_script_factory_; 
    Script();

  private:
    SERIALIZE_DECL
    friend class Abstract::ObjectBase;

    [[nodiscard]] virtual StrongScript cloneImpl() const = 0;

    void SetOwner(const WeakObjectBase& owner);

    eScriptType m_type_;
    WeakObjectBase m_owner_;
    bool       m_b_active_;
  };
} // namespace Engine::Component

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Engine::Script)
BOOST_CLASS_EXPORT_KEY(Engine::Script);
