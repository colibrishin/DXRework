#pragma once
#include <ranges>
#include <map>

#include "egComponent.hpp"
#include <set>
#include <typeindex>

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
			if constexpr (std::is_base_of_v<Component, T>)
			{
				std::shared_ptr<T> component = std::make_shared<T>();
				component->Initialize();

				m_components_.insert_or_assign(typeid(T), std::reinterpret_pointer_cast<Component>(component));
			}
		}

		template <typename T>
		std::weak_ptr<T> GetComponent()
		{
			if constexpr (std::is_base_of_v<Component, T>)
			{
				return std::reinterpret_pointer_cast<T>(m_components_[typeid(T)]);
			}

			return {};
		}

		template <typename T>
		void RemoveComponent()
		{
			if constexpr (std::is_base_of_v<Component, T>)
			{
				m_components_.erase(typeid(T));
			}
		}

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
		bool m_active_ = true;
		std::map<const std::type_index, ComponentPtr> m_components_;
	};

	inline void Object::PreUpdate()
	{
		for (const auto& component : m_components_ | std::views::values)
		{
			component->PreUpdate();
		}
	}

	inline void Object::PreRender()
	{
		for (const auto& component : m_components_ | std::views::values)
		{
			component->PreRender();
		}
	}

	inline void Object::Render()
	{
		for (const auto& component : m_components_ | std::views::values)
		{
			component->Render();
		}
	}

	inline void Object::Update()
	{
		for (const auto& component : m_components_ | std::views::values)
		{
			component->Update();
		}
	}
}
