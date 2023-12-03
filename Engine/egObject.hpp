#pragma once
#include <map>
#include <set>

#include "egActor.hpp"
#include "egComponent.hpp"
#include "egScene.hpp"
#include "egResource.hpp"
#include <boost/make_shared.hpp>

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

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void FixedUpdate(const float& dt) override;
		void OnDeserialized() override;

		void AddResource(const WeakResource& resource)
		{
			m_resources_.insert(resource);
			m_resource_names_.insert(resource.lock()->GetName());
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

				UINT idx = 0;

				while (true)
				{
					if (idx == g_invalid_id)
					{
						throw std::exception("Component ID overflow");
					}

					if (!m_assigned_component_ids_.contains(idx))
					{
						component->SetLocalID(idx);
						m_assigned_component_ids_.insert(idx);
						break;
					}

					idx++;
				}

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

		template <typename T>
		boost::weak_ptr<T> GetComponentByLocal(ComponentID id)
		{
			if (m_assigned_component_ids_.contains(id))
			{
				const auto it = std::find_if(m_priority_sorted_.begin(), m_priority_sorted_.end(), [id](const auto& comp)
				{
					return comp.lock()->GetLocalID() == id;
				});

				if (it != m_priority_sorted_.end())
				{
					return boost::reinterpret_pointer_cast<T>((*it).lock());
				}
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
					RemoveComponentCommon<T>(first);
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

					RemoveComponentCommon<T>(*found);
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
		Object() : Actor(), m_active_(true), m_culled_(true)
		{
		};

	private:
		SERIALIZER_ACCESS

		friend class Engine::Scene;
		template <typename T>
		void RemoveComponentCommon(StrongComponent& value)
		{
			m_priority_sorted_.erase(value);
			m_components_[typeid(T).name()].erase(value);
			m_assigned_component_ids_.erase(value->GetLocalID());

			if (const auto scene = GetScene().lock())
			{
				scene->RemoveCacheComponent(value);
			}

			if (m_components_[typeid(T).name()].empty())
			{
				m_components_.erase(typeid(T).name());
			}
		}

		virtual void OnCollisionEnter(const Engine::Component::Collider& other);
		virtual void OnCollisionContinue(const Engine::Component::Collider& other);
		virtual void OnCollisionExit(const Engine::Component::Collider& other);

		bool m_active_ = true;
		bool m_culled_ = true;

		std::map<const std::string, std::set<StrongComponent, ComponentPriorityComparer>> m_components_;
		std::set<std::wstring> m_resource_names_;

		// Non-serialized
		std::set<ComponentID> m_assigned_component_ids_;
		std::set<WeakComponent, ComponentPriorityComparer> m_priority_sorted_;
		std::set<WeakResource, ResourcePriorityComparer> m_resources_;
	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Abstract::Object)