#pragma once
#include <map>

#include "Source/Runtime/Abstracts/CoreRenderable/Public/Renderable.h"
#include "Source/Runtime/TypeLibrary/Public/TypeLibrary.h"

namespace Engine
{
	template <typename T>
	struct which_script
	{
		static constexpr ScriptSizeType value = T::scptype;
	};

	class Script : public Abstracts::Renderable
	{
	public:
		~Script() override = default;
		explicit Script(ScriptSizeType type, const Weak<Abstracts::ObjectBase>& owner);

		virtual void SetActive(bool active);

		bool GetActive() const
		{
			return m_b_active_;
		}

		Weak<Abstracts::ObjectBase> GetOwner() const
		{
			return m_owner_;
		}

		ScriptSizeType GetScriptType() const
		{
			return m_type_;
		}

		[[nodiscard]] Strong<Script> Clone(const Weak<Abstracts::ObjectBase>& owner) const;

		using ScriptFactoryFunction = std::function<Strong<Script>(const Weak<Abstracts::ObjectBase>&)>;

		template <typename T, typename SLock = std::enable_if_t<std::is_base_of_v<Script, T>>>
		static void Register()
		{
			s_script_factory_[typeid(T).name()] = &T::Create;
		}

		static const std::map<std::string, ScriptFactoryFunction>& GetScriptFactory()
		{
			return s_script_factory_;
		}

	protected:
		inline static std::map<std::string, ScriptFactoryFunction> s_script_factory_;
		Script();

		virtual void OnCollisionEnter(const Weak<Components::Collider>& other) = 0;
		virtual void OnCollisionContinue(const Weak<Components::Collider>& other) = 0;
		virtual void OnCollisionExit(const Weak<Components::Collider>& other) = 0;

	private:
		SERIALIZE_DECL
		friend class Abstracts::ObjectBase;

		[[nodiscard]] virtual Strong<Script> cloneImpl() const = 0;

		void SetOwner(const Weak<Abstracts::ObjectBase>& owner);

		ScriptSizeType              m_type_;
		Weak<Abstracts::ObjectBase> m_owner_;
		bool                        m_b_active_;
	};
} // namespace Engine::Component

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Engine::Script)
