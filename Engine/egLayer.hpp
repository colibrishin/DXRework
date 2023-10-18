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

		void AddGameObject (const StrongObject& obj)
		{
			m_objects_.emplace(obj->GetID(), std::move(obj));
		}

		void RemoveGameObject (uint64_t id)
		{
			m_objects_.erase(id);
		}

		template <typename T>
		std::weak_ptr<T> GetGameObject (uint64_t id)
		{
			for (const auto& object : m_objects_)
			{
				if(object.first == id)
				{
					return std::reinterpret_pointer_cast<T>(object.second);
				}
			}

			return {};
		}

	private:
		eLayerType m_layer_type_;
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
}
