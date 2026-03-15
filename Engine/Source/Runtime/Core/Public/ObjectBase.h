#pragma once
#include <map>
#include <set>
#include <type_traits>

#include "Actor.h"
#include "Scene.h"
#include "Component.h"

#include "ObjectBase.generated.h"

DEFINE_DELEGATE(OnComponentAdded, Engine::Weak<Engine::Abstracts::Component>);
DEFINE_DELEGATE(OnComponentRemoved, Engine::Weak<Engine::Abstracts::Component>);

// Static engine default provided object type, this should be added to every object
#define OBJECT_T(enum_val) static constexpr Engine::eDefObjectType dotype = enum_val;

// Cloning object declaration macro
#define OBJ_CLONE_DECL Engine::Strong<Engine::Abstracts::ObjectBase> cloneImpl() const override;
// Cloning object implementation macro
#define OBJ_CLONE_IMPL(CLASS) Engine::Strong<Engine::Abstracts::ObjectBase> CLASS::cloneImpl() const { return make_managed_shared<CLASS>(*this); }

namespace Engine
{
	enum ENGINE_CORE_API eDefObjectType : uint8_t
	{
		DEF_OBJ_T_UNK = 0,
		DEF_OBJ_T_NONE,
		DEF_OBJ_T_CAMERA,
		DEF_OBJ_T_LIGHT,
		DEF_OBJ_T_OBSERVER,
		DEF_OBJ_T_TEXT,
	};
}

namespace Engine::Abstracts
{
	// Abstract base class for objects
	ECLASS(abstract, serialize)
	class ENGINE_CORE_API ObjectBase : public Actor
	{
		GENERATE_BODY
	public:
		DelegateOnComponentAdded onComponentAdded;
		DelegateOnComponentRemoved onComponentRemoved;

		~ObjectBase() override = default;

		void Initialize() override;
        virtual void BeginPlay( const float dt );
        virtual void EndPlay( const float dt );
#if WITH_EDITOR
		void OnUIUpdate(UIContext* const parent, const float dt) override;
#endif
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void PostRender(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostUpdate(const float dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		[[nodiscard]] Strong<ObjectBase> Clone(bool register_scene = true, std::vector<Strong<ObjectBase>>* out_child = nullptr) const;

		template <typename T, typename... Args> requires (std::is_base_of_v<Component, T> && !std::is_same_v<Component, T>)
		Weak<T> AddComponent(Args&&... args)
		{
			const auto type = T::StaticTypeHash();

			if (const auto comp = checkComponent(type).lock())
			{
				return managed_static_pointer_cast<T>( comp );
			}

			const auto thisObject = GetSharedPtr<ObjectBase>();
			Strong<T> component = make_managed_shared<T>(thisObject, std::forward<Args>(args)...);
			component->Initialize();

			addComponentImpl(component, type);

			return component;
		}

		const std::set<Weak<Component>, ComponentPriorityComparer>& GetAllComponents();

		template <typename T> requires (std::is_base_of_v<Component, T> && !std::is_same_v<Component, T>)
		Weak<T> GetComponent()
		{
			if constexpr (std::is_base_of_v<Component, T>)
			{
				if (!m_components_.contains(T::StaticTypeHash()))
				{
					return {};
				}

				const auto& comp = m_components_[T::StaticTypeHash()];

				return managed_static_pointer_cast<T>( comp );
			}

#pragma warning(disable: 4702)
			return {};
#pragma warning(default: 4702)
		}

		template <typename T> requires (std::is_base_of_v<Component, T> && !std::is_same_v<Component, T>)
		void RemoveComponent()
		{
			removeComponent(T::StaticTypeHash());
		}

		void SetName(const std::string_view name) override;
		void SetActive(bool active);
		void SetCulled(bool culled);

		bool           GetActive() const;
		bool           GetCulled() const;
		eDefObjectType GetObjectType() const;

		Weak<ObjectBase>              GetParent() const;
		Weak<ObjectBase>              GetChild(const std::string& name) const;
		Weak<ObjectBase>              GetChild(LocalActorID id) const;
		std::vector<Weak<ObjectBase>> GetChildren() const;

		// Add child to object, if immediate flag is set, the child will be added in frame.
		void AddChild(const Weak<ObjectBase>& p_child, bool immediate = false);
		void addChildImpl(const Weak<ObjectBase>& child);

		// Detach child from object, if immediate flag is set, the child will be detached in frame.
		bool DetachChild(LocalActorID id, bool immediate = false);
		void detachChildImpl(LocalActorID id);

	protected:
		explicit ObjectBase(const eDefObjectType type = static_cast<eDefObjectType>(0))
			: Actor(),
			m_parent_id_(g_invalid_id),
			m_type_(type),
			m_active_(true),
			m_culled_(true) {};

	private:
		friend class Scene;
		friend class Layer;

		// Overridable function for derived object clone behavior.
		[[nodiscard]] virtual Strong<ObjectBase> cloneImpl() const = 0;

		// Check whether the component is already added to the object.
		Weak<Component> checkComponent(ComponentType type);

		// Add component to the scene cache.
		template <typename T, typename CLock = std::enable_if_t<std::is_base_of_v<Component, T>>>
		void addComponentToSceneCache(const Strong<T>& component)
		{
			if (const auto scene = GetScene().lock())
			{
				scene->AddCacheComponent<T>(component);
			}
		}

		template <typename T, typename CLock = std::enable_if_t<std::is_base_of_v<Component, T>>>
		void removeComponentFromSceneCache(const Strong<T>& component)
		{
			if (const auto scene = GetScene().lock())
			{
				scene->RemoveCacheComponent<T>(component);
			}
		}

		// Add pre-existing component to the object.
		Weak<Component> addComponent(const Strong<Component>& component);

		// Remove component from the object. Cached component at the scene should be removed manually.
		void removeComponent(ComponentType type);
		// Remove specific component from the object. Cached component at the scene should be removed manually.
		void removeComponent(GlobalEntityID id);
		// Remove component from the object finally, notify task scheduler to remove the component.
		void removeComponentImpl(ComponentType type, const Strong<Component>& comp);

		// Commit the component to the object.
		void addComponentImpl(const Strong<Component>& component, ComponentType type);

		EPROPERTY()
		LocalActorID m_parent_id_;

		EPROPERTY()
		std::vector<LocalActorID> m_children_;

		EPROPERTY()
		eDefObjectType m_type_;

		EPROPERTY()
		bool m_active_ = true;

		EPROPERTY()
		bool m_culled_ = true;

		EPROPERTY()
		std::map<ComponentType, Strong<Component>> m_components_;

		// Non-serialized
#if WITH_EDITOR
	public:
		friend struct ConstructorAccess;
		void UpdateUIText();
		void OnNameChanged();

	private:
		bool m_b_add_component_dialog_opened_ = false;
		bool m_b_child_dialog_ = false;
		bool m_b_child_add_dialog_ = false;
#endif
		Weak<ObjectBase>                                     m_parent_;
		std::map<LocalActorID, Weak<ObjectBase>>             m_children_cache_;
		std::set<LocalComponentID>                           m_assigned_component_ids_;
		std::set<Weak<Component>, ComponentPriorityComparer> m_cached_component_;
	};
} // namespace Engine::Abstracts

namespace Engine 
{
	template struct ENGINE_CORE_API FactoryTemplate<Engine::Abstracts::ObjectBase>;
	using ObjectFactory = FactoryTemplate<Engine::Abstracts::ObjectBase>;
}

#define REGISTER_OBJECT(TYPE) Engine::ObjectFactory::Register<##TYPE##>();
#define UNREGISTER_OBJECT(TYPE) Engine::ObjectFactory::Unregister<##TYPE##>();