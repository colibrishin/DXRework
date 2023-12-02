#pragma once
#include <string>
#include <memory>

#include "egType.hpp"
#include <boost/enable_shared_from_this.hpp>
#include <boost/serialization/access.hpp>

namespace Engine::Abstract
{
	class Entity : public boost::enable_shared_from_this<Entity>
	{
	public:
		Entity(const Entity& other) = default;
		virtual ~Entity() = default;

		bool operator ==(const Entity& other) const
		{
			return GetID() == other.GetID();
		}
		
		void SetName(const std::wstring& name) { m_name_ = name; }

		EntityID GetID() const { return reinterpret_cast<EntityID>(this); }
		std::wstring GetName() const { return m_name_; }

		template <typename T>
		boost::weak_ptr<T> GetWeakPtr()
		{
			return boost::reinterpret_pointer_cast<T>(shared_from_this());
		}

		template <typename T>
		boost::shared_ptr<T> GetSharedPtr()
		{
			return boost::reinterpret_pointer_cast<T>(shared_from_this());
		}

		virtual void Initialize() = 0;
		virtual void PreUpdate(const float& dt) = 0;
		virtual void Update(const float& dt) = 0;
		virtual void FixedUpdate(const float& dt) = 0;

	protected:
		Entity() = default;

		void SetGarbage(const bool bGarbage) { m_bGarbage_ = bGarbage; }

	private:
		friend class boost::serialization::access;
		std::wstring m_name_;
		bool m_bGarbage_;

	};
}
