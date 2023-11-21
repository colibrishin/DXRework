#pragma once
#include <map>
#include <ranges>

#include <set>
#include <typeindex>
#include "egComponent.hpp"

#include "egRenderable.hpp"
#include "egResource.hpp"

namespace Engine::Component
{
	class Collider;
}

namespace Engine::Abstract
{
	using WeakComponentPtr = std::weak_ptr<Component>;
	using ComponentPtr = std::shared_ptr<Component>;
	using WeakResourcePtr = std::weak_ptr<Resource>;

	class Object : public Renderable
	{
	public:
		~Object() override = default;
		Object(const Object&) = default;

		void AddResource(const WeakResourcePtr& resource)
		{
			m_resources_.insert(resource);
		}

		template <typename T>
		void AddComponent()
		{
			if constexpr (std::is_base_of_v<Component, T>)
			{
				const auto thisObject = std::reinterpret_pointer_cast<Object>(shared_from_this());

				std::shared_ptr<T> component = std::make_shared<T>(thisObject);
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
		std::weak_ptr<T> GetComponent(uint64_t id)
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
		void RemoveComponent(const uint64_t id)
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

		template <typename T = Component>
		void DispatchComponentEvent(const std::shared_ptr<T>& thisComp, const std::shared_ptr<T>& otherComp);

		void SetLayer(eLayerType type);

		void SetActive(bool active) { m_active_ = active; }
		void SetCulled(bool culled) { m_culled_ = culled; }

		bool GetActive() const { return m_active_; }
		eLayerType GetLayer() const { return m_layer_; }

	protected:
		Object() : m_layer_(LAYER_NONE), m_active_(true), m_culled_(true) {};

	public:
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void FixedUpdate(const float& dt) override;

	private:
		struct ResourcePriorityComparer
		{
			bool operator()(const WeakResourcePtr& Left, const WeakResourcePtr& Right) const
			{
				if (Left.lock()->GetPriority() != Right.lock()->GetPriority())
				{
					return Left.lock()->GetPriority() < Right.lock()->GetPriority();
				}

				return Left.lock()->GetID() < Right.lock()->GetID();
			}
		};

		struct ComponentPriorityComparer
		{
			bool operator()(const WeakComponentPtr& Left, const WeakComponentPtr& Right) const
			{
				if (Left.lock()->GetPriority() != Right.lock()->GetPriority())
				{
					return Left.lock()->GetPriority() > Right.lock()->GetPriority();
				}

				return Left.lock()->GetID() > Right.lock()->GetID();
			}
		};

		virtual void OnCollisionEnter(const Engine::Component::Collider& other);
		virtual void OnCollisionContinue(const Engine::Component::Collider& other);
		virtual void OnCollisionExit(const Engine::Component::Collider& other);

		eLayerType m_layer_;
		bool m_active_ = true;
		bool m_culled_ = true;

		std::map<const std::type_index, std::set<ComponentPtr, ComponentPriorityComparer>> m_components_;
		std::set<WeakComponentPtr, ComponentPriorityComparer> m_priority_sorted_;

		std::set<WeakResourcePtr, ResourcePriorityComparer> m_resources_;
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
