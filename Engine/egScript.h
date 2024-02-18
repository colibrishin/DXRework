#pragma once
#include "egCommon.hpp"

namespace Engine
{
  class Script : public Abstract::Renderable
  {
  public:
    explicit Script(const WeakObject& owner);

    void       SetActive(bool active);
    bool       GetActive() const { return m_b_active_; }
    WeakObject GetOwner() const { return m_owner_; }

    using ScriptFactoryFunction = std::function<StrongScript(const WeakObject&)>;

    template <typename T, typename SLock = std::enable_if_t<std::is_base_of_v<Script, T>>>
    static void Register() { s_script_factory_[typeid(T).name()] = &T::Create; }

    static std::map<std::string, ScriptFactoryFunction>& GetScriptFactory() { return s_script_factory_; }

  protected:
    inline static std::map<std::string, ScriptFactoryFunction> s_script_factory_; 
    Script();

  private:
    SERIALIZER_ACCESS

    WeakObject m_owner_;
    bool       m_b_active_;
  };
} // namespace Engine::Component

BOOST_CLASS_EXPORT_KEY(Engine::Script);
