#include "../Public/Layer.h"
#include "Layer.generated.h"

#if WITH_EDITOR
#include "UIInterface.h"
#endif

#include "SingletonSpinLock/Public/SingletonSpinLock.h"

#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.h"

namespace Engine
{
	Layer::Layer(const LayerSizeType type)
		: m_layer_type_(type),
		  m_cache_lock_idx_(SingletonSpinLock::GetInstance().Register()) { }

	Layer::~Layer() {}

	void Layer::Initialize()
    {}

    void Layer::BeginPlay( const float dt )
    {
		for (const auto& object : m_objects_)
		{
            if ( !object->GetActive() )
            {
                continue;
            }

            if ( object->GetParent().lock() )
            {
                continue;
            }

            object->BeginPlay( dt );
		}
    }

    void Layer::EndPlay( const float dt )
    {
        for ( const auto &object : m_objects_ )
        {
            if ( !object->GetActive() )
            {
                continue;
            }

            if ( object->GetParent().lock() )
            {
                continue;
            }

            object->EndPlay( dt );
        }
	}

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

#if WITH_EDITOR
	void Layer::OnUIUpdate(UIContext* const parent, const float dt)
	{
		if (parent)
		{
			UIInterface& ui = UIInterfaceAccessor::GetInterface();
			*parent += ui.NewTreeNode({GetName()});
            *parent |= ui.NewDragAndDropTarget(
                    { "OBJECT",
                      [ this ]( void *ptr )
                      {
                          if ( const Strong<Abstracts::ObjectBase> *scary_ptr =
                                       static_cast<Strong<Abstracts::ObjectBase> *>( ptr ) )
                          {
                              if ( const Strong<Abstracts::ObjectBase> &parent = ( *scary_ptr )->GetParent().lock() )
                              {
                                  parent->DetachChild( ( *scary_ptr )->GetLocalID() );
                              }

                              if ( const Strong<Scene> &scene = ( *scary_ptr )->GetScene().lock() )
                              {
                                  scene->ChangeLayer( m_layer_type_, ( *scary_ptr )->GetID() );
                              }
                          }
                      } } );

			for (const auto& object : m_objects_)
			{
			    if ( object->GetParent().expired() )
			    {
                    ( *parent |= ui.NewButton( { m_ui_info_.temporaryStrings[ std::format( "{}remove", object->GetID() ) ] } ) )
                            .SetFunction( [ object ]() 
					{
                        if ( const Strong<Scene> &scene = object->GetScene().lock() )
                        {
                            scene->RemoveGameObject( object->GetID(), object->GetLayer() );
						}
					} );
                    *parent |= ui.NewSameLine( {} );
			        *parent |= ui.NewSelectable({object->m_ui_info_.label, object->m_ui_info_.dialogOpened});
                    *parent |= ui.NewDragAndDropSource(
                            { "OBJECT", object->m_ui_info_.label, &object, sizeof( decltype( object ) ) } );

			        if (object->m_ui_info_.dialogOpened)
			        {
			            object->OnUIUpdate(parent, dt);   
			        }
			    }
			}

			--*parent;
		}
	}
#endif

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
#if WITH_EDITOR
        m_ui_info_.temporaryStrings[ std::format( "{}remove", obj->GetID() ) ] = std::format( 
			"Remove##{}_remove_button", 
			obj->GetID() );
#endif
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

#if WITH_EDITOR
            m_ui_info_.temporaryStrings.erase( std::format( "{}remove", locked->GetID() ) );
#endif
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

    void Layer::clear()
	{
	    m_objects_.clear();
		m_concurrent_weak_objects_cache_.clear();
		m_weak_objects_cache_.clear();
	}

    Layer::Layer() :
		m_layer_type_(0),
		m_cache_lock_idx_(SingletonSpinLock::GetInstance().Register()){}
} // namespace Engine
