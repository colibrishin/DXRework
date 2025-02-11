#pragma once
#include <map>

#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "Source/Runtime/CoreEntity/Public/Renderable.h"
#include "Script.generated.h"

namespace Engine
{
	ECLASS(serialize)
	class ENGINE_CORE_API Script : public Abstracts::Renderable
	{
		GENERATE_BODY
	public:
		~Script() override = default;

		virtual void SetActive(bool active);

		bool GetActive() const
		{
			return m_b_active_;
		}

		Weak<Abstracts::ObjectBase> GetOwner() const
		{
			return m_owner_;
		}

		[[nodiscard]] Strong<Script> Clone(const Weak<Abstracts::ObjectBase>& owner) const;

	protected:
		explicit Script(const Weak<Abstracts::ObjectBase>& owner);
		Script();

		virtual void OnCollisionEnter(const Weak<Components::Collider>& other) = 0;
		virtual void OnCollisionContinue(const Weak<Components::Collider>& other) = 0;
		virtual void OnCollisionExit(const Weak<Components::Collider>& other) = 0;

	private:
		friend struct ConstructorAccess;
		friend class Abstracts::ObjectBase;

		[[nodiscard]] virtual Strong<Script> cloneImpl() const = 0;

		void SetOwner(const Weak<Abstracts::ObjectBase>& owner);
		
		EPROPERTY()
		Weak<Abstracts::ObjectBase> m_owner_;
		
		EPROPERTY()
		bool                        m_b_active_;
	};
} // namespace Engine::Components

namespace Engine
{
	using ScriptFactory = FactoryTemplate<Engine::Script, const Engine::Weak<Engine::Abstracts::ObjectBase>&>;
}

#define REGISTER_SCRIPT(TYPE) Engine::ScriptFactory::Register<##TYPE##>();
#define UNREGISTER_SCRIPT(TYPE) Engine::ScriptFactory::Unregister<##TYPE##>();

// Cloning script declaration macro
#define SCRIPT_CLONE_DECL Engine::Strong<Engine::Script> cloneImpl() const override;
// Cloning script implementation macro
#define SCRIPT_CLONE_IMPL(CLASS) Engine::Strong<Engine::Script> CLASS::cloneImpl() const { return boost::make_shared<CLASS>(*this); }