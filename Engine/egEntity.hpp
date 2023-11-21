#pragma once
#include <string>
#include <memory>

#include "egType.hpp"

namespace Engine::Abstract
{
	class Entity : public std::enable_shared_from_this<Entity>
	{
	public:
		Entity(const Entity& other) = default;
		virtual ~Entity() = default;

		bool operator ==(const Entity& other) const
		{
			return GetID() == other.GetID();
		}

		EntityID GetID() const { return reinterpret_cast<EntityID>(this); }
		void SetName(const std::wstring& name) { m_name_ = name; }
		std::wstring GetName() const { return m_name_; }

		template <typename T>
		std::weak_ptr<T> GetWeakPtr()
		{
			return std::reinterpret_pointer_cast<T>(shared_from_this());
		}

		template <typename T>
		std::shared_ptr<T> GetSharedPtr()
		{
			return std::reinterpret_pointer_cast<T>(shared_from_this());
		}

		virtual void Initialize() = 0;
		virtual void PreUpdate(const float& dt) = 0;
		virtual void Update(const float& dt) = 0;
		virtual void FixedUpdate(const float& dt) = 0;

	protected:
		Entity() = default;

	private:
		std::wstring m_name_;
	};
}
