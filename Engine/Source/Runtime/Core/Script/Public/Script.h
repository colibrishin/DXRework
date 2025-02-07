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
		explicit Script(const Weak<Abstracts::ObjectBase>& owner);

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
		Script();

		virtual void OnCollisionEnter(const Weak<Components::Collider>& other) = 0;
		virtual void OnCollisionContinue(const Weak<Components::Collider>& other) = 0;
		virtual void OnCollisionExit(const Weak<Components::Collider>& other) = 0;

	private:
		friend class ScriptFactory;
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
	using ScriptGeneratorSignature = std::function<Strong<Script>(const Weak<Abstracts::ObjectBase>&)>;

	struct ENGINE_CORE_API ScriptFactory
	{
	public:
		static Engine::ScriptGeneratorSignature GetGenerator(Engine::ScriptType type)
		{
			if (m_script_generators_.contains(type))
			{
				return m_script_generators_.at(type);
			}

			return {};
		}

		static const std::unordered_map<Engine::ScriptType, Engine::ScriptGeneratorSignature>& GetGenerators() 
		{
			return m_script_generators_;
		}

		template <typename T> requires std::is_base_of_v<Engine::Script, T>
		static void Register()
		{
			m_script_generators_.emplace(T::StaticTypeHash(), &ScriptFactory::Create<T>);
		}

		template <typename T> requires std::is_base_of_v<Engine::Script, T>
		static void Unregister()
		{
			if (m_script_generators_.contains(T::StaticTypeHash()))
			{
				m_script_generators_.erase(T::StaticTypeHash());
			}
		}

		template <typename T> requires std::is_base_of_v<Engine::Script, T>
		static Engine::Strong<Engine::Script> Create(const Engine::Weak<Engine::Abstracts::ObjectBase>& owner)
		{
			return boost::shared_ptr<T>(new T(owner));
		}

	private:
		static std::unordered_map<Engine::ScriptType, Engine::ScriptGeneratorSignature> m_script_generators_;
	};
}

#define REGISTER_SCRIPT(TYPE) Engine::ScriptFactory::Register<##TYPE##>();
#define UNREGISTER_SCRIPT(TYPE) Engine::ScriptFactory::Unregister<##TYPE##>();

// Cloning script declaration macro
#define SCRIPT_CLONE_DECL Engine::Strong<Engine::Script> cloneImpl() const override;
// Cloning script implementation macro
#define SCRIPT_CLONE_IMPL(CLASS) Engine::Strong<Engine::Script> CLASS::cloneImpl() const { return boost::make_shared<CLASS>(*this); }