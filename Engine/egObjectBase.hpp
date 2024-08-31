#pragma once
#include "egActor.h"
#include "egCommon.hpp"
#include "egComponent.h"
#include "egDelegate.hpp"
#include "egResource.h"
#include "egScene.hpp"
#include "egScript.h"

DEFINE_DELEGATE(OnComponentAdded, Engine::Weak<Engine::Abstract::Component>)
DEFINE_DELEGATE(OnComponentRemoved, Engine::Weak<Engine::Abstract::Component>)

namespace Engine::Abstract
{
	// Abstract base class for objects
	class ObjectBase : public Actor
	{
	public:
		DelegateOnComponentAdded onComponentAdded;
		DelegateOnComponentRemoved onComponentRemoved;

		~ObjectBase() override = default;

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostUpdate(const float& dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;
		void OnImGui() override;

		[[nodiscard]] StrongObjectBase Clone(bool register_scene = true) const;

		template <typename T, typename... Args, typename CLock = std::enable_if_t<std::is_base_of_v<Component, T>>>
		boost::weak_ptr<T> AddComponent(Args&&... args)
		{
			const auto type = which_component<T>::value;

			if (const auto comp = checkComponent(type).lock())
			{
				return boost::static_pointer_cast<T>(comp);
			}

			const auto thisObject = GetSharedPtr<ObjectBase>();

			boost::shared_ptr<T> component =
					boost::make_shared<T>(thisObject, std::forward<Args>(args)...);
			component->Initialize();

			addComponentImpl(component, type);
			addComponentToSceneCache<T>(component);

			onComponentAdded.Broadcast(component);

			return component;
		}

		template <typename T, typename SLock = std::enable_if_t<std::is_base_of_v<Script, T>>>
		boost::weak_ptr<T> AddScript(const std::string& name = "")
		{
			const auto type = which_script<T>::value;

			if (m_scripts_.contains(which_script<T>::value))
			{
				return boost::static_pointer_cast<T>(m_scripts_[which_script<T>::value]);
			}

			boost::shared_ptr<T> script = boost::make_shared<T>(GetSharedPtr<ObjectBase>());
			script->SetName(name);
			addScriptImpl(script, type);
			addScriptToSceneCache<T>(script);

			return script;
		}

		template <typename T, typename SLock = std::enable_if_t<std::is_base_of_v<Script, T>>>
		boost::weak_ptr<T> GetScript(const std::string& name = "")
		{
			if (m_scripts_.contains(which_script<T>::value))
			{
				return boost::static_pointer_cast<T>(m_scripts_[which_script<T>::value]);
			}

			return {};
		}

		template <typename T, typename SLock = std::enable_if_t<std::is_base_of_v<Script, T>>>
		void RemoveScript()
		{
			removeScriptFromSceneCache<T>(m_scripts_[which_script<T>::value]);
			removeScriptImpl(which_script<T>::value);
		}

		const std::set<WeakComponent, ComponentPriorityComparer>& GetAllComponents();
		const std::vector<WeakScript>&                            GetAllScripts();

		template <typename T>
		boost::weak_ptr<T> GetComponent()
		{
			if constexpr (std::is_base_of_v<Component, T>)
			{
				if (!m_components_.contains(which_component<T>::value))
				{
					return {};
				}

				const auto& comp = m_components_[which_component<T>::value];

				return boost::static_pointer_cast<T>(comp);
			}

			return {};
		}

		template <typename T, typename CLock = std::enable_if_t<std::is_base_of_v<Component, T>>>
		void RemoveComponent()
		{
			removeComponentFromSceneCache<T>(m_components_[which_component<T>::value]);
			removeComponent(which_component<T>::value);
		}

		void SetActive(bool active);
		void SetCulled(bool culled);
		void SetImGuiOpen(bool open);

		bool           GetActive() const;
		bool           GetCulled() const;
		bool&          GetImGuiOpen();
		eDefObjectType GetObjectType() const;

		WeakObjectBase              GetParent() const;
		WeakObjectBase              GetChild(const std::string& name) const;
		WeakObjectBase              GetChild(LocalActorID id) const;
		std::vector<WeakObjectBase> GetChildren() const;

		// Add child to object, if immediate flag is set, the child will be added in frame.
		void AddChild(const WeakObjectBase& p_child, bool immediate = false);
		void addChildImpl(const WeakObjectBase& child);

		// Detach child from object, if immediate flag is set, the child will be detached in frame.
		bool DetachChild(LocalActorID id, bool immediate = false);
		void detachChildImpl(LocalActorID id);

	protected:
		explicit ObjectBase(eDefObjectType type = DEF_OBJ_T_NONE)
			: Actor(),
			  m_parent_id_(g_invalid_id),
			  m_type_(type),
			  m_active_(true),
			  m_culled_(true),
			  m_imgui_open_(false) { };

		virtual void OnCollisionEnter(const StrongCollider& other);
		virtual void OnCollisionContinue(const StrongCollider& other);
		virtual void OnCollisionExit(const StrongCollider& other);

	private:
		SERIALIZE_DECL
		friend class Scene;
		friend class Manager::Graphics::ShadowManager;

		// Overridable function for derived object clone behavior.
		[[nodiscard]] virtual StrongObjectBase cloneImpl() const = 0;

		// Check whether the component is already added to the object.
		WeakComponent checkComponent(eComponentType type);
		WeakScript    checkScript(eScriptType type);

		// Add component to the scene cache.
		template <typename T, typename CLock = std::enable_if_t<std::is_base_of_v<Component, T>>>
		void addComponentToSceneCache(const boost::shared_ptr<T>& component)
		{
			if (const auto scene = GetScene().lock())
			{
				scene->AddCacheComponent<T>(component);
			}
		}

		template <typename T, typename CLock = std::enable_if_t<std::is_base_of_v<Component, T>>>
		void removeComponentFromSceneCache(const boost::shared_ptr<T>& component)
		{
			if (const auto scene = GetScene().lock())
			{
				scene->RemoveCacheComponent<T>(component);
			}
		}

		// Add script to the scene cache.
		template <typename T, typename CLock = std::enable_if_t<std::is_base_of_v<Script, T>>>
		void addScriptToSceneCache(const boost::shared_ptr<T>& script)
		{
			if (const auto scene = GetScene().lock())
			{
				scene->AddCacheScript<T>(script);
			}
		}

		// Remove script from the scene cache.
		template <typename T, typename CLock = std::enable_if_t<std::is_base_of_v<Script, T>>>
		void removeScriptFromSceneCache(const boost::shared_ptr<T>& script)
		{
			if (const auto scene = GetScene().lock())
			{
				scene->RemoveCacheScript<T>(script);
			}
		}

		void removeScript(eScriptType type);
		void removeScriptImpl(eScriptType type);

		// Add pre-existing component to the object.
		WeakComponent addComponent(const StrongComponent& component);

		// Add pre-existing script to the object.
		WeakScript addScript(const StrongScript& script);

		// Remove component from the object. Cached component at the scene should be removed manually.
		void removeComponent(eComponentType type);
		// Remove specific component from the object. Cached component at the scene should be removed manually.
		void removeComponent(GlobalEntityID id);
		// Remove component from the object finally, notify task scheduler to remove the component.
		void removeComponentImpl(eComponentType type, const StrongComponent& comp);

		// Commit the component to the object.
		void addComponentImpl(const StrongComponent& component, eComponentType type);
		// Commit the script to the object.
		void addScriptImpl(const StrongScript& script, eScriptType type);

		LocalActorID              m_parent_id_;
		std::vector<LocalActorID> m_children_;
		eDefObjectType            m_type_;
		bool                      m_active_ = true;
		bool                      m_culled_ = true;

		bool m_imgui_open_            = false;
		bool m_imgui_children_open_   = false;
		bool m_imgui_components_open_ = false;

		std::map<eComponentType, StrongComponent> m_components_;
		std::map<eScriptType, StrongScript>       m_scripts_;

		// Non-serialized
		WeakObjectBase                                     m_parent_;
		std::map<LocalActorID, WeakObjectBase>             m_children_cache_;
		std::set<LocalComponentID>                         m_assigned_component_ids_;
		std::set<WeakComponent, ComponentPriorityComparer> m_cached_component_;
		std::vector<WeakScript>                            m_cached_script_;
	};
} // namespace Engine::Abstract

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Engine::Abstract::ObjectBase)
BOOST_CLASS_EXPORT_KEY(Engine::Abstract::ObjectBase)
