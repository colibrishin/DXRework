#pragma once
#include <map>
#include <ranges>

#include <set>
#include <typeindex>

#include "egActor.hpp"
#include "egComponent.hpp"

#include "egRenderable.hpp"
#include "egResource.hpp"
#include "egType.hpp"

namespace Engine::Component
{
	class Collider;
}

namespace Engine::Abstract
{
	class Object : public Actor
	{
	public:
		~Object() override = default;

		void AddResource(const WeakResource& resource)
		{
			m_resources_.insert(resource);
		}

		template <typename T, typename... Args>
		void AddComponent(Args&&... args)
		{
			if constexpr (std::is_base_of_v<Component, T>)
			{
				const auto thisObject = std::reinterpret_pointer_cast<Object>(shared_from_this());

				std::shared_ptr<T> component = std::make_shared<T>(thisObject, std::forward<Args>(args)...);
				component->Initialize();

				m_components_[typeid(T)].insert(std::reinterpret_pointer_cast<Component>(component));
				m_priority_sorted_.insert(std::reinterpret_pointer_cast<Component>(component));
			}
		}

		template <typename T>
		std::weak_ptr<T> GetComponent()
		{
			if constexpr (std::is_base_of_v<Component, T>)
			{
				if (!m_components_.contains(typeid(T)))
				{
					return {};
				}

				const auto& comp_set = m_components_[typeid(T)];

				return std::reinterpret_pointer_cast<T>(*comp_set.begin());
			}

			return {};
		}

		template <typename T>
		std::set<std::weak_ptr<T>, WeakComparer<T>> GetComponents()
		{
			if constexpr (std::is_base_of_v<Component, T>)
			{
				if (!m_components_.contains(typeid(T)))
				{
					return {};
				}

				const auto& comp_set = m_components_[typeid(T)];

				std::set<std::weak_ptr<T>, WeakComparer<T>> result;

				for (const auto& comp : comp_set)
				{
					result.insert(std::reinterpret_pointer_cast<T>(comp));
				}

				return result;
			}

			return {};
		}

		template <typename T>
		std::weak_ptr<T> GetComponent(EntityID id)
		{
			if constexpr (std::is_base_of_v<Component, T>)
			{
				if (!m_components_.contains(typeid(T)))
				{
					return {};
				}

				const auto& comp_set = m_components_[typeid(T)];

				const auto found = std::find_if(comp_set.begin(), comp_set.end(), [id](const auto& comp)
				{
					return comp.lock()->GetID() == id;
				});

				if (found == comp_set.end())
				{
					return {};
				}

				return std::reinterpret_pointer_cast<T>(*found);
			}

			return {};
		}

		template <typename T>
		void RemoveComponent()
		{
			if constexpr (std::is_base_of_v<Component, T>)
			{
				if (m_components_.contains(typeid(T)))
				{
					const auto& comp_set = m_components_[typeid(T)];

					if (comp_set.empty())
					{
						return;
					}

					const auto first = *comp_set.begin();
					m_priority_sorted_.erase(first);
					m_components_[typeid(T)].erase(first);

					if (comp_set.empty())
					{
						m_components_.erase(typeid(T));
					}
				}
			}
		}

		template <typename T>
		void RemoveComponent(const EntityID id)
		{
			if constexpr (std::is_base_of_v<Component, T>)
			{
				if (m_components_.contains(typeid(T)))
				{
					const auto& comp_set = m_components_[typeid(T)];

					if (comp_set.empty())
					{
						return;
					}

					const auto found = std::find_if(comp_set.begin(), comp_set.end(), [id](const auto& comp)
					{
						return comp.lock()->GetID() == id;
					});

					if (found == comp_set.end())
					{
						return;
					}

					m_priority_sorted_.erase(found);
					m_components_[typeid(T)].erase(found);

					if (comp_set.empty())
					{
						m_components_.erase(typeid(T));
					}
				}
			}
		}

		template <typename T>
		std::weak_ptr<T> GetResource() const
		{
			if constexpr (std::is_base_of_v<Resource, T>)
			{
				for (const auto& resource : m_resources_)
				{
					if (const auto locked = resource.lock())
					{
						if (const auto t = std::dynamic_pointer_cast<T>(locked))
						{
							return t;
						}
					}
				}
			}

			return {};
		}

		template <typename T>
		std::weak_ptr<T> GetResource(const std::wstring& name) const
		{
			if constexpr (std::is_base_of_v<Resource, T>)
			{
				for (const auto& resource : m_resources_)
				{
					if (const auto locked = resource.lock())
					{
						if (const auto t = std::dynamic_pointer_cast<T>(locked))
						{
							if (!name.empty() && t->GetName() == name)
							{
								return t;
							}
						}
					}
				}
			}

			return {};
		}

		template <typename T = Component>
		void DispatchComponentEvent(const std::shared_ptr<T>& thisComp, const std::shared_ptr<T>& otherComp);

		void SetActive(bool active) { m_active_ = active; }
		void SetCulled(bool culled) { m_culled_ = culled; }

		bool GetActive() const { return m_active_; }

	protected:
		Object(const WeakScene& initial_scene, const eLayerType initial_layer) : Actor(initial_scene, initial_layer), m_active_(true), m_culled_(true) {};

	public:
		void Initialize() final;
		virtual void Initialize_INTERNAL() = 0;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void FixedUpdate(const float& dt) override;

	private:
		virtual void OnCollisionEnter(const Engine::Component::Collider& other);
		virtual void OnCollisionContinue(const Engine::Component::Collider& other);
		virtual void OnCollisionExit(const Engine::Component::Collider& other);

	protected:
		void OnCreate() override;
		void OnDestroy() override;
		void OnLayerChanging() override;
		void OnLayerChanged() override;
		void OnSceneChanging() override;
		void OnSceneChanged() override;

	private:
		bool m_active_ = true;
		bool m_culled_ = true;

		std::map<const std::type_index, std::set<StrongComponent, ComponentPriorityComparer>> m_components_;
		std::set<WeakComponent, ComponentPriorityComparer> m_priority_sorted_;
		std::set<WeakResource, ResourcePriorityComparer> m_resources_;
	};

	inline void Object::PreUpdate(const float& dt)
	{
		for (const auto& component : m_priority_sorted_)
		{
			if (const auto locked = component.lock())
			{
				locked->PreUpdate(dt);
			}
			else
			{
				m_priority_sorted_.erase(component);
			}
		}

		for (const auto& resource : m_resources_)
		{
			if (const auto locked = resource.lock())
			{
				locked->PreUpdate(dt);
			}
			else
			{
				m_resources_.erase(resource);
			}
		}
	}

	inline void Object::PreRender(const float dt)
	{
		for (const auto& component : m_priority_sorted_)
		{
			if (const auto locked = component.lock())
			{
				locked->PreRender(dt);
			}
			else
			{
				m_priority_sorted_.erase(component);
			}
		}

		for (const auto& resource : m_resources_)
		{
			if (const auto locked = resource.lock())
			{
				locked->PreRender(dt);
			}
			else
			{
				m_resources_.erase(resource);
			}
		}
	}

	inline void Object::Update(const float& dt)
	{
		for (const auto& component : m_priority_sorted_)
		{
			if (const auto locked = component.lock())
			{
				locked->Update(dt);
			}
			else
			{
				m_priority_sorted_.erase(component);
			}
		}

		for (const auto& resource : m_resources_)
		{
			if (const auto locked = resource.lock())
			{
				locked->Update(dt);
			}
			else
			{
				m_resources_.erase(resource);
			}
		}
	}
}
