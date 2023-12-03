#pragma once
#include "egCommon.hpp"
#include "egObject.hpp"
#include "egRenderable.hpp"

namespace Engine
{
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

		void AddGameObject(const StrongObject& obj)
		{
			if(m_objects_.contains(obj->GetID()))
			{
				return;
			}

			m_objects_.insert_or_assign(obj->GetID(), obj);
			m_weak_objects_cache_.push_back(obj);
		}

		void RemoveGameObject(EntityID id)
		{
			if (m_objects_.contains(id))
			{
				m_objects_.erase(id);
				std::erase_if(m_weak_objects_cache_, [id](const auto& obj) { return obj.lock()->GetID() == id; });
			}
		}

		WeakObject GetGameObject (EntityID id) const
		{
			for (const auto& object : m_objects_)
			{
				if(object.first == id)
				{
					return object.second;
				}
			}

			return {};
		}

		const std::vector<WeakObject>& GetGameObjects()
		{
			return m_weak_objects_cache_;
		}

	private:
		friend class boost::serialization::access;

		eLayerType m_layer_type_;
		std::vector<WeakObject> m_weak_objects_cache_;
		std::map<const EntityID, StrongObject> m_objects_;

	};
}
