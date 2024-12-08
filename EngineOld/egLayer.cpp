#include "pch.h"
#include "egLayer.h"
#include "egObject.hpp"

SERIALIZE_IMPL
(
 Engine::Layer,
 _ARTAG(_BSTSUPER(Engine::Abstract::Renderable))
 _ARTAG(m_layer_type_)
 _ARTAG(m_objects_)
)

namespace Engine
{
	Layer::Layer(eLayerType type)
		: m_layer_type_(type) { }

	void Layer::Initialize() {}

	void Layer::PreUpdate(const float& dt)
	{
		for (const auto& object : m_objects_)
		{
			if (!object->GetActive())
			{
				continue;
			}

			if (object->GetParent().lock())
			{
				continue;
			}

			object->PreUpdate(dt);
		}
	}

	void Layer::Update(const float& dt)
	{
		for (const auto& object : m_objects_)
		{
			if (!object->GetActive())
			{
				continue;
			}

			if (object->GetParent().lock())
			{
				continue;
			}

			object->Update(dt);
		}
	}

	void Layer::PreRender(const float& dt)
	{
		for (const auto& object : m_objects_)
		{
			if (!object->GetActive())
			{
				continue;
			}

			if (object->GetParent().lock())
			{
				continue;
			}

			object->PreRender(dt);
		}
	}

	void Layer::Render(const float& dt)
	{
		for (const auto& object : m_objects_)
		{
			if (!object->GetActive())
			{
				continue;
			}

			if (object->GetParent().lock())
			{
				continue;
			}

			object->Render(dt);
		}
	}

	void Layer::PostRender(const float& dt)
	{
		for (const auto& object : m_objects_)
		{
			if (!object->GetActive())
			{
				continue;
			}

			if (object->GetParent().lock())
			{
				continue;
			}

			object->PostRender(dt);
		}
	}

	void Layer::FixedUpdate(const float& dt)
	{
		for (const auto& object : m_objects_)
		{
			if (!object->GetActive())
			{
				continue;
			}

			if (object->GetParent().lock())
			{
				continue;
			}

			object->FixedUpdate(dt);
		}
	}

	void Layer::PostUpdate(const float& dt)
	{
		for (const auto& object : m_objects_)
		{
			if (!object->GetActive())
			{
				continue;
			}

			if (object->GetParent().lock())
			{
				continue;
			}

			object->PostUpdate(dt);
		}
	}

	void Layer::OnSerialized()
	{
		Renderable::OnSerialized();

		for (const auto& object : m_objects_)
		{
			object->OnSerialized();
		}
	}

	void Layer::OnDeserialized()
	{
		Renderable::OnDeserialized();

		// rebuild cache
		for (const auto& object : m_objects_)
		{
			object->OnDeserialized();
			m_weak_objects_cache_.insert({object->GetID(), object});
		}
	}

	void Layer::AddGameObject(const StrongObjectBase& obj)
	{
		{
			decltype(m_weak_objects_cache_)::const_accessor it;

			if (m_weak_objects_cache_.find(it, obj->GetID()))
			{
				return;
			}
		}

		m_objects_.push_back(obj);
		m_weak_objects_cache_.insert({obj->GetID(), obj});
	}

	void Layer::RemoveGameObject(GlobalEntityID id)
	{
		WeakObjectBase obj;

		{
			ConcurrentWeakObjGlobalMap::const_accessor acc;

			if (m_weak_objects_cache_.find(acc, id))
			{
				obj = acc->second;
			}
		}

		if (const auto locked = obj.lock())
		{
			std::erase_if(m_objects_, [&locked](const StrongObjectBase& value)
				{
					return value == locked;
				});

			m_weak_objects_cache_.erase(id);
		}
	}

	WeakObjectBase Layer::FindGameObject(GlobalEntityID id) const
	{
		ConcurrentWeakObjGlobalMap::const_accessor acc;

		if (m_weak_objects_cache_.find(acc, id))
		{
			return acc->second;
		}

		if (const auto& it = std::ranges::find_if
					(
					 m_objects_,
					 [id](const auto& obj)
					 {
						 return obj->GetID() == id;
					 }
					);
			it != m_objects_.end())
		{
			return *it;
		}

		return {};
	}

	WeakObjectBase Layer::FindGameObjectByLocalID(const LocalActorID id) const
	{
		if (const auto& it = std::ranges::find_if
					(
					 m_objects_,
					 [id](const auto& obj)
					 {
						 return obj->GetLocalID() == id;
					 }
					);
			it != m_objects_.end())
		{
			return *it;
		}

		return {};
	}

	ConcurrentWeakObjVec Layer::GetGameObjects() const
	{
		ConcurrentWeakObjVec result;

		for (const auto& obj : m_weak_objects_cache_ | std::views::values)
		{
			result.push_back(obj);
		}

		return result;
	}

	Layer::Layer()
		: m_layer_type_(LAYER_NONE) {}
} // namespace Engine
