#pragma once
#include <ranges>
#include <map>

#include "egComponent.hpp"
#include <set>
#include <typeindex>

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
				if (m_components_.contains(typeid(T)))
				{
					return;
				}

				const auto thisObject = std::reinterpret_pointer_cast<Object>(shared_from_this());

				std::shared_ptr<T> component = std::make_shared<T>(thisObject);
				component->Initialize();

				m_components_.insert_or_assign(typeid(T), std::reinterpret_pointer_cast<Component>(component));
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

				return std::reinterpret_pointer_cast<T>(m_components_[typeid(T)]);
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
					m_components_.erase(typeid(T));
					return;
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

		void SetActive(bool active) { m_active_ = active; }
		bool GetActive() const { return m_active_; }

	protected:
		Object() = default;

	public:
		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Render() override;

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

		virtual void OnCollisionEnter(const Engine::Component::Collider& other);
		virtual void OnCollisionExit(const Engine::Component::Collider& other);

		bool m_active_ = true;
		std::map<const std::type_index, ComponentPtr> m_components_;
		std::set<WeakResourcePtr, ResourcePriorityComparer> m_resources_;
	};

	inline void Object::PreUpdate()
	{
		for (const auto& component : m_components_ | std::views::values)
		{
			component->PreUpdate();
		}

		for (const auto& resource : m_resources_)
		{
			if (const auto locked = resource.lock())
			{
				locked->PreUpdate();
			}
		}
	}

	inline void Object::PreRender()
	{
		for (const auto& component : m_components_ | std::views::values)
		{
			component->PreRender();
		}

		for (const auto& resource : m_resources_)
		{
			if (const auto locked = resource.lock())
			{
				locked->PreRender();
			}
		}
	}

	inline void Object::Render()
	{
		for (const auto& component : m_components_ | std::views::values)
		{
			component->Render();
		}

		for (const auto& resource : m_resources_)
		{
			if (const auto locked = resource.lock())
			{
				locked->Render();
			}
		}
	}

	inline void Object::Update()
	{
		for (const auto& component : m_components_ | std::views::values)
		{
			component->Update();
		}

		for (const auto& resource : m_resources_)
		{
			if (const auto locked = resource.lock())
			{
				locked->Update();
			}
		}
	}
}
