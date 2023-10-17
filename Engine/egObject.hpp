#pragma once
#include "egComponent.hpp"
#include <set>

#include "egRenderable.hpp"

namespace Engine::Abstract
{
	using WeakComponentPtr = std::weak_ptr<Component>;
	using ComponentPtr = std::shared_ptr<Component>;

	class Object : public Renderable
	{
	public:
		~Object() override = default;
		Object(const Object&) = default;

		template <typename T>
		void AddComponent()
		{
			std::shared_ptr<T> component = std::make_shared<T>();
			component->Initialize();
			m_components_.emplace(component);
		}

		template <typename T>
		std::weak_ptr<T> GetComponent()
		{
			for (const auto& component : m_components_)
			{
				if (const auto casted = std::dynamic_pointer_cast<T>(component))
				{
					return casted;
				}
			}

			return {};
		}

		template <typename T>
		void RemoveComponent()
		{
			m_components_.erase(
				std::remove_if(
					m_components_.begin(), 
					m_components_.end(), 
					[](const ComponentPtr& component)
			{
					return std::dynamic_pointer_cast<T>(component) != nullptr;
			}));
		}

	protected:
		Object() = default;

	public:
		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Render() override;

	private:
		std::set<ComponentPtr> m_components_;
	};

	inline void Object::PreUpdate()
	{
		for (const auto& component : m_components_)
		{
			component->PreUpdate();
		}
	}

	inline void Object::PreRender()
	{
		for (const auto& component : m_components_)
		{
			component->PreRender();
		}
	}

	inline void Object::Render()
	{
		for (const auto& component : m_components_)
		{
			component->Render();
		}
	}

	inline void Object::Update()
	{
		for (const auto& component : m_components_)
		{
			component->Update();
		}
	}
}
