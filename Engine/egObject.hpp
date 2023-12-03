#pragma once
#include <map>
#include <ranges>

#include <set>
#include <typeindex>

#include "egActor.hpp"
#include "egComponent.hpp"

#include "egRenderable.hpp"
#include "egResource.hpp"
#include <boost/make_shared.hpp>

#include "egScene.hpp"

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
				const auto thisObject = boost::reinterpret_pointer_cast<Object>(shared_from_this());

				boost::shared_ptr<T> component = boost::make_shared<T>(thisObject, std::forward<Args>(args)...);
				component->Initialize();

				m_components_[typeid(T).name()].insert(boost::reinterpret_pointer_cast<Component>(component));
				m_priority_sorted_.insert(boost::reinterpret_pointer_cast<Component>(component));

				if (const auto scene = GetScene().lock())
				{
					scene->AddCacheComponent(component);
				}
			}
		}

		template <typename T>
		boost::weak_ptr<T> GetComponent()
		{
			if constexpr (std::is_base_of_v<Component, T>)
			{
				if (!m_components_.contains(typeid(T).name()))
				{
					return {};
				}

				const auto& comp_set = m_components_[typeid(T).name()];

				return boost::reinterpret_pointer_cast<T>(*comp_set.begin());
			}

			return {};
		}

		const std::set<WeakComponent, ComponentPriorityComparer>& GetAllComponents() const
		{
			return m_priority_sorted_;
		}

		template <typename T>
		std::set<boost::weak_ptr<T>, WeakComparer<T>> GetComponents()
		{
			if constexpr (std::is_base_of_v<Component, T>)
			{
				if (!m_components_.contains(typeid(T).name()))
				{
					return {};
				}

				const auto& comp_set = m_components_[typeid(T).name()];

				std::set<boost::weak_ptr<T>, WeakComparer<T>> result;

				for (const auto& comp : comp_set)
				{
					result.insert(boost::reinterpret_pointer_cast<T>(comp));
				}

				return result;
			}

			return {};
		}

		template <typename T>
		boost::weak_ptr<T> GetComponent(EntityID id)
		{
			if constexpr (std::is_base_of_v<Component, T>)
			{
				if (!m_components_.contains(typeid(T).name()))
				{
					return {};
				}

				const auto& comp_set = m_components_[typeid(T).name()];

				const auto found = std::find_if(comp_set.begin(), comp_set.end(), [id](const auto& comp)
				{
					return comp.lock()->GetID() == id;
				});

				if (found == comp_set.end())
				{
					return {};
				}

				return boost::reinterpret_pointer_cast<T>(*found);
			}

			return {};
		}

		template <typename T>
		void RemoveComponent()
		{
			if constexpr (std::is_base_of_v<Component, T>)
			{
				if (m_components_.contains(typeid(T).name()))
				{
					const auto& comp_set = m_components_[typeid(T).name()];

					if (comp_set.empty())
					{
						return;
					}

					const auto first = *comp_set.begin();
					m_priority_sorted_.erase(first);
					m_components_[typeid(T).name()].erase(first);

					if (const auto scene = GetScene().lock())
					{
						scene->RemoveCacheComponent(first);
					}

					if (comp_set.empty())
					{
						m_components_.erase(typeid(T).name());
					}
				}
			}
		}

		template <typename T>
		void RemoveComponent(const EntityID id)
		{
			if constexpr (std::is_base_of_v<Component, T>)
			{
				if (m_components_.contains(typeid(T).name()))
				{
					const auto& comp_set = m_components_[typeid(T).name()];

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
					m_components_[typeid(T).name()].erase(found);

					if (const auto scene = GetScene().lock())
					{
						scene->RemoveCacheComponent(*found);
					}

					if (comp_set.empty())
					{
						m_components_.erase(typeid(T).name());
					}
				}
			}
		}

		template <typename T>
		boost::weak_ptr<T> GetResource() const
		{
			if constexpr (std::is_base_of_v<Resource, T>)
			{
				for (const auto& resource : m_resources_)
				{
					if (const auto locked = resource.lock())
					{
						if (const auto t = boost::dynamic_pointer_cast<T>(locked))
						{
							return t;
						}
					}
				}
			}

			return {};
		}

		template <typename T>
		boost::weak_ptr<T> GetResource(const std::wstring& name) const
		{
			if constexpr (std::is_base_of_v<Resource, T>)
			{
				for (const auto& resource : m_resources_)
				{
					if (const auto locked = resource.lock())
					{
						if (const auto t = boost::dynamic_pointer_cast<T>(locked))
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

		template <typename T>
		void DispatchComponentEvent(T& lhs, T& rhs);

		void SetActive(bool active) { m_active_ = active; }
		void SetCulled(bool culled) { m_culled_ = culled; }

		bool GetActive() const { return m_active_; }

	protected:
		Object() : Actor(), m_active_(true), m_culled_(true) {};
		void AfterDeserialized() override;

	public:
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void FixedUpdate(const float& dt) override;

	private:
		virtual void OnCollisionEnter(const Engine::Component::Collider& other);
		virtual void OnCollisionContinue(const Engine::Component::Collider& other);
		virtual void OnCollisionExit(const Engine::Component::Collider& other);

		friend class boost::serialization::access;
		bool m_active_ = true;
		bool m_culled_ = true;

		std::map<const std::string, std::set<StrongComponent, ComponentPriorityComparer>> m_components_;

		// Non-serialized
		std::set<WeakComponent, ComponentPriorityComparer> m_priority_sorted_;
		std::set<WeakResource, ResourcePriorityComparer> m_resources_;
	};
}
