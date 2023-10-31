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
		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Render() override;
		void FixedUpdate() override;

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

	inline void Layer::PreUpdate()
	{
		for (const auto& object : m_objects_ | std::views::values)
		{
			if (!object->GetActive())
			{
				continue;
			}

			object->PreUpdate();
		}
	}

	inline void Layer::Update()
	{
		for (const auto& object : m_objects_ | std::views::values)
		{
			if (!object->GetActive())
			{
				continue;
			}

			object->Update();
		}
	}

	inline void Layer::PreRender()
	{
		for (const auto& object : m_objects_ | std::views::values)
		{
			if (!object->GetActive())
			{
				continue;
			}

			object->PreRender();
		}
	}

	inline void Layer::Render()
	{
		for (const auto& object : m_objects_ | std::views::values)
		{
			if (!object->GetActive())
			{
				continue;
			}

			object->Render();
		}
	}

	inline void Layer::FixedUpdate()
	{
		for (const auto& object : m_objects_ | std::views::values)
		{
			if (!object->GetActive())
			{
				continue;
			}

			object->FixedUpdate();
		}
	}
}
