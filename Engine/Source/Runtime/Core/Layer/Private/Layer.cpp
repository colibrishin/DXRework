#include "../Public/Layer.h"
#include "Layer.generated.h"
#include "UIInterface.h"

#include "SingletonSpinLock/Public/SingletonSpinLock.h"

#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.h"

namespace Engine
{
	Layer::Layer(const LayerSizeType type)
		: m_layer_type_(type),
		  m_cache_lock_idx_(SingletonSpinLock::GetInstance().Register()) { }

	Layer::~Layer() {}

	void Layer::Initialize() {}

	void Layer::PreUpdate(const float dt)
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

	void Layer::Update(const float dt)
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

	void Layer::PreRender(const float dt)
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

	void Layer::Render(const float dt)
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

	void Layer::PostRender(const float dt)
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

	void Layer::FixedUpdate(const float dt)
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

	void Layer::PostUpdate(const float dt)
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

	void Layer::OnUIUpdate(UIContext* const parent, const float dt)
	{
#if WITH_EDITOR
		if (parent)
		{
			UIInterface& ui = UIInterfaceAccessor::GetInterface();
			*parent += ui.NewTreeNode({GetName()});

			for (const auto& object : m_objects_)
			{
			    if ( object->GetParent().expired() )
			    {
			        *parent |= ui.NewSelectable({object->m_ui_info_.label, object->m_ui_info_.dialogOpened});

			        if (object->m_ui_info_.dialogOpened)
			        {
			            object->OnUIUpdate(parent, dt);   
			        }
			    }
			}

			--*parent;
		}
#endif
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
		SpinLockToken token = SingletonSpinLock::GetInstance().Lock(m_cache_lock_idx_);
		for (const auto& object : m_objects_)
		{
			object->OnDeserialized();
			m_weak_objects_cache_.emplace(object->GetID(), object);
			m_concurrent_weak_objects_cache_.emplace(object->GetID(), object);
		}
	}

	void Layer::AddGameObject(const Strong<Abstracts::ObjectBase>& obj)
	{
		{
			SpinLockToken token = SingletonSpinLock::GetInstance().Lock(m_cache_lock_idx_);
			if (m_weak_objects_cache_.contains(obj->GetID()))
			{
				return;
			}
		}

		m_objects_.push_back(obj);
		m_weak_objects_cache_.emplace(obj->GetID(), obj);
		m_concurrent_weak_objects_cache_.emplace(obj->GetID(), obj);
	}

	void Layer::RemoveGameObject(GlobalEntityID id)
	{
		SpinLockToken token = SingletonSpinLock::GetInstance().Lock(m_cache_lock_idx_);
		if (!m_weak_objects_cache_.contains(id))
		{
			return;
		}

		if (const auto locked = m_weak_objects_cache_[id].lock())
		{
			std::erase_if(m_objects_, [&locked](const Strong<Abstracts::ObjectBase>& value)
				{
					return value == locked;
				});

			m_weak_objects_cache_.erase(id);
			m_concurrent_weak_objects_cache_.erase(id);
		}
	}

	Weak<Abstracts::ObjectBase> Layer::FindGameObject(GlobalEntityID id) const
	{
		if (m_weak_objects_cache_.contains(id))
		{
			return m_weak_objects_cache_.at(id);
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

	Weak<Abstracts::ObjectBase> Layer::FindGameObjectByLocalID(const LocalActorID id) const
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

	ConcurrentWeakObjVec Layer::GetGameObjectsConcurrent() const
	{
		ConcurrentWeakObjVec result;

		for (const auto& obj : m_concurrent_weak_objects_cache_ | std::views::values)
		{
			result.push_back(obj);
		}

		return result;
	}

	WeakObjVec Layer::GetGameObjects() const
	{
		SpinLockToken token = SingletonSpinLock::GetInstance().Lock(m_cache_lock_idx_);
		WeakObjVec result;
		for (const auto& obj : m_weak_objects_cache_ | std::views::values)
		{
			result.emplace_back(obj);
		}
		return result;
	}

	Layer::Layer() :
		m_layer_type_(0),
		m_cache_lock_idx_(SingletonSpinLock::GetInstance().Register()){}
} // namespace Engine
