#pragma once
#include "egCommon.hpp"
#include "egObject.hpp"
#include "egRenderable.hpp"

namespace Engine
{
	using StrongObject = std::shared_ptr<Abstract::Object>;
	using WeakObject = std::weak_ptr<Abstract::Object>;

	class Layer : public Abstract::Renderable
	{
	public:
		Layer(eLayerType type) : m_layer_type_(type) {}
		virtual ~Layer() override = default;

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void FixedUpdate(const float& dt) override;

		template <typename T>
		void AddGameObject (const StrongObject& obj)
		{
			if constexpr (std::is_base_of_v<Abstract::Object, T>)
			{
				if(m_objects_.contains(obj->GetID()))
				{
					return;
				}

				m_objects_.insert_or_assign(obj->GetID(), obj);
				m_weak_objects_cache_.push_back(obj);
			}
		}

		template <typename T>
		void RemoveGameObject (uint64_t id)
		{
			if constexpr (std::is_base_of_v<Abstract::Object, T>)
			{
				if (m_objects_.contains(id))
				{
					m_objects_.erase(id);
					m_weak_objects_cache_.erase(std::remove_if(m_weak_objects_cache_.begin(), m_weak_objects_cache_.end(), [id](const auto& obj) { return obj.lock()->GetID() == id; }), m_weak_objects_cache_.end());
				}
			}
		}

		template <typename T>
		std::weak_ptr<T> GetGameObject (uint64_t id)
		{
			if constexpr (std::is_base_of_v<Abstract::Object, T>)
			{
				for (const auto& object : m_objects_)
				{
					if(object.first == id)
					{
						return std::reinterpret_pointer_cast<T>(object.second);
					}
				}
			}

			return {};
		}

		const std::vector<WeakObject>& GetGameObjects()
		{
			return m_weak_objects_cache_;
		}

	private:
		eLayerType m_layer_type_;
		std::vector<WeakObject> m_weak_objects_cache_;
		std::map<const uint64_t, StrongObject> m_objects_;

	};

	inline void Layer::Initialize()
	{
	}

	inline void Layer::PreUpdate(const float& dt)
	{
		for (const auto& object : m_objects_ | std::views::values)
		{
			if (!object->GetActive())
			{
				continue;
			}

			object->PreUpdate(dt);
		}
	}

	inline void Layer::Update(const float& dt)
	{
		for (const auto& object : m_objects_ | std::views::values)
		{
			if (!object->GetActive())
			{
				continue;
			}

			object->Update(dt);
		}
	}

	inline void Layer::PreRender(const float dt)
	{
		for (const auto& object : m_objects_ | std::views::values)
		{
			if (!object->GetActive())
			{
				continue;
			}

			object->PreRender(dt);
		}
	}

	inline void Layer::Render(const float dt)
	{
		for (const auto& object : m_objects_ | std::views::values)
		{
			if (!object->GetActive())
			{
				continue;
			}

			object->Render(dt);
		}
	}

	inline void Layer::FixedUpdate(const float& dt)
	{
		for (const auto& object : m_objects_ | std::views::values)
		{
			if (!object->GetActive())
			{
				continue;
			}

			object->FixedUpdate(dt);
		}
	}
}
