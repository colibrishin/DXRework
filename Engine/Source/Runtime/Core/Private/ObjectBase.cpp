#include "ObjectBase.h"

#include <any>

#include "Collider.h"
#include "TaskScheduler.h"

#if WITH_EDITOR
#include "Scene.h"
#include "Object.h"
#include "Transform.h"
#include "Rigidbody.h"
#endif

namespace Engine::Abstracts
{
	void ObjectBase::SetName(const std::string_view name)
	{
		Actor::SetName(name);
#if WITH_EDITOR
		UpdateUIText();
#endif
	}

	void ObjectBase::SetActive(bool active)
	{
		m_active_ = active;
	}

	void ObjectBase::SetCulled(bool culled)
	{
		m_culled_ = culled;
	}

	bool ObjectBase::GetActive() const
	{
		return m_active_;
	}

	bool ObjectBase::GetCulled() const
	{
		return m_culled_;
	}

	eDefObjectType ObjectBase::GetObjectType() const
	{
		return m_type_;
	}

	Weak<ObjectBase> ObjectBase::GetParent() const
	{
		if (m_parent_id_ == g_invalid_id)
		{
			return {};
		}

		return m_parent_;
	}

	Weak<ObjectBase> ObjectBase::GetChild(const std::string& name) const
	{
		for (const auto& child : m_children_cache_ | std::views::values)
		{
			if (const auto locked = child.lock();
				locked && locked->GetName() == name)
			{
				return locked;
			}
		}

		return {};
	}

	Weak<ObjectBase> ObjectBase::GetChild(const LocalActorID id) const
	{
		if (id == g_invalid_id)
		{
			return {};
		}

		if (m_children_cache_.contains(id))
		{
			return m_children_cache_.at(id);
		}

		return {};
	}

	std::vector<Weak<ObjectBase>> ObjectBase::GetChildren() const
	{
		std::vector<Weak<ObjectBase>> out;

		for (const auto& child : m_children_cache_ | std::views::values)
		{
			if (const auto locked = child.lock())
			{
				out.push_back(locked);
			}
		}

		return out;
	}

	void ObjectBase::AddChild(const Weak<ObjectBase>& p_child, const bool immediate )
	{
		if (const auto child = p_child.lock();
			!child || child == GetSharedPtr<ObjectBase>())
		{
			return;
		}

		if (immediate)
		{
			addChildImpl(p_child);
			return;
		}

		Managers::TaskScheduler::GetInstance().AddTask
				(
				 TASK_ADD_CHILD, {p_child}, [this](auto& params, const auto dt)
				 {
					 const auto& cast_child = std::any_cast<Weak<ObjectBase>>(params[0]);
					 addChildImpl(cast_child);
				 }
				);
	}

	void ObjectBase::addChildImpl(const Weak<ObjectBase>& child)
	{
		if (const auto locked = child.lock())
		{
			m_children_.push_back(locked->GetLocalID());
			m_children_cache_.insert({locked->GetLocalID(), child});

			if (locked->m_parent_id_ != g_invalid_id)
			{
				if (const auto& parent = locked->m_parent_.lock())
				{
					parent->DetachChild(locked->GetLocalID(), true);
				}
			}

			locked->m_parent_id_ = GetLocalID();
			locked->m_parent_    = GetSharedPtr<ObjectBase>();
		}
	}

	bool ObjectBase::DetachChild(const LocalActorID id, const bool immediate)
	{
		if (id == g_invalid_id)
		{
			return false;
		}

		if (m_children_cache_.contains(id))
		{
			if (immediate)
			{
				detachChildImpl(id);
			}
			else
			{
				Managers::TaskScheduler::GetInstance().AddTask
						(
						 TASK_REM_CHILD, {id}, [this](auto& params, const auto dt)
						 {
							 detachChildImpl(std::any_cast<LocalActorID>(params[0]));
						 }
						);
			}

			return true;
		}

		return false;
	}

	void ObjectBase::detachChildImpl(const LocalActorID id)
	{
		if (m_children_cache_.contains(id))
		{
			if (const auto locked = m_children_cache_[id].lock())
			{
				locked->m_parent_id_ = g_invalid_id;
				locked->m_parent_.reset();
			}

			m_children_cache_.erase(id);
			std::erase(m_children_, id);
		}
	}

	Weak<Abstracts::Component> ObjectBase::checkComponent(const ComponentType type)
	{
		if (m_components_.contains(type))
		{
			return m_components_[type];
		}

		return {};
	}

	Weak<Component> ObjectBase::addComponent(const Strong<Component>& component)
	{
		const auto type = component->GetTypeHash();

		if (const auto comp = checkComponent(type).lock())
		{
			return comp;
		}

		// Remove the component from the previous owner.
		if (const auto prev = component->GetOwner().lock();
			prev && prev != GetSharedPtr<ObjectBase>())
		{
			prev->removeComponent(component->GetID());
		}

		// Change the owner of the component. Since the component is already added to the cache, skipping the uncaching.
		component->SetOwner(GetSharedPtr<ObjectBase>());
		if (!component->IsInitialized())
		{
			component->Initialize();
		}

		// Add the component to the object.
		addComponentImpl(component, type);

		if (const Strong<Scene>& scene = GetScene().lock())
		{
			addComponentToSceneCache(component);
		}

		onComponentAdded.Broadcast(component);

		return component;
	}

#if WITH_EDITOR
	void ObjectBase::UpdateUIText()
	{
        m_ui_info_.label = std::format( "{} {} {}", GetPrettyTypeName(), GetName(), GetID() );
        m_ui_info_.temporaryStrings[ "child_title" ]   = std::format( "Children of {}({})...", GetName(), GetID() );
        m_ui_info_.temporaryStrings[ "child_listbox" ] = std::format( "Children##ChildrenListBox{}", GetID() );
	}
	void ObjectBase::OnNameChanged()
	{
		UpdateUIText();
	}
#endif

	void ObjectBase::removeComponentImpl(const ComponentType type, const Strong<Component>& comp)
	{
		onComponentRemoved.Broadcast(comp);

		Managers::TaskScheduler::GetInstance().AddTask
				(
				 TASK_REM_COMPONENT,
				 {GetSharedPtr<ObjectBase>(), comp, type},
				 [](const std::vector<std::any>& params, const float)
				 {
					 const auto& obj  = std::any_cast<Strong<ObjectBase>>(params[0]);
					 const auto& comp = std::any_cast<Strong<Component>>(params[1]);
					 const auto& type = std::any_cast<ComponentType>(params[2]);

					 obj->m_assigned_component_ids_.erase(comp->GetLocalID());
					 obj->m_cached_component_.erase(comp);
					 obj->m_components_.erase(type);
				 }
				);
	}

	void ObjectBase::removeComponent(const ComponentType type)
	{
		if (m_components_.contains(type))
		{
			const auto& comp = m_components_[type];

		    removeComponentFromSceneCache(comp);
			removeComponentImpl(type, comp);
		}
	}

	void ObjectBase::removeComponent(const GlobalEntityID id)
	{
		for (const auto& [type, comp] : m_components_)
		{
			if (comp->GetID() == id)
			{
				removeComponentImpl(type, comp);
				break;
			}
		}
	}

	void ObjectBase::addComponentImpl(const Strong<Component>& component, ComponentType type)
	{
		m_components_.emplace(type, component);

		UINT idx = 0;

		while (true)
		{
			if (idx == g_invalid_id)
			{
				throw std::exception("Component ID overflow");
			}

			if (!m_assigned_component_ids_.contains(idx))
			{
				component->SetLocalID(idx);
				m_assigned_component_ids_.insert(idx);
				break;
			}

			idx++;
		}

		m_cached_component_.insert(component);
	    addComponentToSceneCache(component);

	    onComponentAdded.Broadcast(component);
	}

	void ObjectBase::Render(const float dt)
	{
		for (const auto& child : m_children_cache_ | std::views::values)
		{
			if (const auto locked = child.lock())
			{
				if (!locked->GetActive())
				{
					continue;
				}

				locked->Render(dt);
			}
		}
	}

	void ObjectBase::PostRender(const float dt)
	{
		for (const auto& child : m_children_cache_ | std::views::values)
		{
			if (const auto locked = child.lock())
			{
				if (!locked->GetActive())
				{
					continue;
				}

				locked->PostRender(dt);
			}
		}
	}

	void ObjectBase::FixedUpdate(const float dt)
	{
		for (const auto& component : m_components_ | std::views::values)
		{
			if (!component->GetActive())
			{
				continue;
			}

			component->FixedUpdate(dt);
		}

		for (const auto& child : m_children_cache_ | std::views::values)
		{
			if (const auto locked = child.lock())
			{
				if (!locked->GetActive())
				{
					continue;
				}

				locked->FixedUpdate(dt);
			}
		}
	}

	void ObjectBase::PostUpdate(const float dt)
	{
		for (const auto& component : m_components_ | std::views::values)
		{
			if (!component->GetActive())
			{
				continue;
			}

			component->PostUpdate(dt);
		}

		for (const auto& child : m_children_cache_ | std::views::values)
		{
			if (const auto locked = child.lock())
			{
				if (!locked->GetActive())
				{
					continue;
				}

				locked->PostUpdate(dt);
			}
		}
	}

	void ObjectBase::OnSerialized()
	{
		Actor::OnSerialized();

		for (const auto& comp : m_components_ | std::views::values)
		{
			comp->OnSerialized();
		}
	}

	void ObjectBase::OnDeserialized()
	{
		Actor::OnDeserialized();

#if WITH_EDITOR
		UpdateUIText();
#endif

		for (const auto& comp : m_components_ | std::views::values)
		{
			comp->SetOwner(GetSharedPtr<ObjectBase>());
			comp->OnDeserialized();
			m_assigned_component_ids_.insert(comp->GetLocalID());
			m_cached_component_.insert(comp);
		}
	}

	Strong<ObjectBase> ObjectBase::Clone( bool register_scene, std::vector<Strong<ObjectBase>> *out_child ) const
	{
		const auto& cloned = cloneImpl();

		// Clone components and scripts
		cloned->m_components_.clear();

		// Erase the copied pointers that indicates original object.
		cloned->m_assigned_component_ids_.clear();
		cloned->m_cached_component_.clear();
		cloned->m_components_.clear();

		// Copy components
		for (const auto& comp : m_components_ | std::views::values)
		{
			const auto& cloned_comp = comp->Clone();
			cloned->addComponent(cloned_comp);
		}

		// Keep intact with the parent.

		// Clone children
		cloned->m_children_.clear();
		cloned->m_children_cache_.clear();
		// Detach from the parent for not getting removed from the original object.
        cloned->m_parent_    = {};
        cloned->m_parent_id_ = g_invalid_id;

		for (const auto& child : m_children_cache_ | std::views::values)
		{
			if (const auto locked = child.lock())
			{
				const auto cloned_child = locked->Clone( register_scene, out_child );
				cloned->AddChild( cloned_child, true );

				if (out_child)
				{
                    out_child->emplace_back( cloned_child );
				}
			}
		}

		// Add to the scene finally.
		if (const auto& scene = GetScene().lock();
			scene && register_scene)
		{
			scene->AddGameObject(GetLayer(), cloned);
		}

		// Keep in mind that this object is yielded, not added to the scene yet.
		return cloned;
	}

	const std::set<Weak<Abstracts::Component>, ComponentPriorityComparer>& ObjectBase::GetAllComponents()
	{
		return m_cached_component_;
	}

	void ObjectBase::Initialize()
	{
#if WITH_EDITOR
		UpdateUIText();
#endif
    }

    void ObjectBase::BeginPlay( const float dt )
    {
		for (const auto& comp : m_cached_component_)
		{
            for ( const auto &component : m_components_ | std::views::values )
            {
                if ( !component->GetActive() )
                {
                    continue;
                }

                component->BeginPlay( dt );
            }

            for ( const auto &child : m_children_cache_ | std::views::values )
            {
                if ( const auto locked = child.lock() )
                {
                    if ( !locked->GetActive() )
                    {
                        continue;
                    }

                    locked->BeginPlay( dt );
                }
            }
		}
    }

    void ObjectBase::EndPlay( const float dt )
    {
        for ( const auto &comp : m_cached_component_ )
        {
            for ( const auto &component : m_components_ | std::views::values )
            {
                if ( !component->GetActive() )
                {
                    continue;
                }

                component->EndPlay( dt );
            }

            for ( const auto &child : m_children_cache_ | std::views::values )
            {
                if ( const auto locked = child.lock() )
                {
                    if ( !locked->GetActive() )
                    {
                        continue;
                    }

                    locked->EndPlay( dt );
                }
            }
        }
	}

#if WITH_EDITOR
	void ObjectBase::OnUIUpdate(UIContext* const parent, const float dt)
	{
		if (parent)
		{
			IUIAPI& ui = g_ui_accessor.GetInterface();

            if ( m_ui_info_.dialogOpened )
            {
                if ( UIContext context = IUIAPI::NewContext(
                        ui.NewDialog( this, "ObjectBaseDialog", { GetName(), m_ui_info_.dialogOpened } ) ) )
                {
                    Actor::OnUIUpdate( &context, dt );
                    ( context |= ui.NewButton( this, "CloneButton", { "Clone" } ) )
                            .SetFunction( [ & ]()
                            {
                                const auto &_ = Clone( true );
                            } );

                    ( context |= ui.NewButton( this, "AddComponentButton", { "Add Component" } ) )
                            .SetFunction( [ & ]()
                            {
                                m_b_add_component_dialog_opened_ = !m_b_add_component_dialog_opened_;
                            } );

                    ( context |= ui.NewButton( this, "ChildrenButton", { "Children" } ) )
                            .SetFunction( [ & ]()
                            {
                                m_b_child_dialog_ = !m_b_child_dialog_;
                            } );

                    if ( m_b_add_component_dialog_opened_ )
                    {
                        if ( UIContext add_com_context = IUIAPI::NewContext( ui.NewDialog( this,
                            "AddComponentDialog",
                            { "Add Component dialog", m_b_add_component_dialog_opened_ } ) ) )
                        {
                            const auto &internalComponentTemplate = [&] <typename T> requires ( std::is_base_of_v<
                                        Component, T> )()
                            {
                                ( add_com_context |= ui.NewButton( this,
                                                                   std::format(
                                                                           "Add{}ComponentButton",
                                                                           T::StaticTypeName() ),
                                                                   { T::StaticTypeName() } ) ).SetFunction( [&]()
                                {
                                    AddComponent<T>();
                                } );
                            };

                            internalComponentTemplate.operator()<Components::Collider>();
                            internalComponentTemplate.operator()<Components::Transform>();
                            internalComponentTemplate.operator()<Components::Rigidbody>();

                            for ( const auto &[ type, predicate ] : ComponentFactory::GetGenerators() )
                            {
                                ( add_com_context |= ui.NewButton( this,
                                                                   std::format(
                                                                           "Add{}ComponentButton",
                                                                           type->GetTypeName() ),
                                                                   { type->GetTypeName() } ) )
                                        .SetFunction( [ & ]()
                                        {
                                            addComponent( predicate( GetSharedPtr<ObjectBase>() ) );
                                        } );
                            }
                        };
                    }

                    context += ui.NewTreeNode( this, "ComponentTreeNode", { "Components" } );
                    context += [&]()
                    {
                        for ( auto it = m_cached_component_.begin(); it != m_cached_component_.end(); ++it )
                        {
                            const Weak<Component> &w_component = *it;
                            const ptrdiff_t        idx         = std::distance( m_cached_component_.begin(), it );
                            if ( const Strong<Component> &component = w_component.lock() )
                            {
                                ( context |= ui.
                                            NewButton( this, std::format( "ComponentRemove{}", idx ), { "Remove" } ) ).
                                        SetFunction( [this, type = component->GetTypeHash()]()
                                        {
                                            removeComponent( type );
                                        } );
                                context |= ui.NewSameLine( this, std::format( "SLComponent{}", idx ), {} );
                                context |= ui.NewSelectable( this,
                                                             std::format( "ComponentSelectable{}", idx ),
                                                             { component->m_ui_info_.label,
                                                               component->m_ui_info_.dialogOpened } );
                            };
                        }
                    };
                }

                for ( auto it = m_cached_component_.begin(); it != m_cached_component_.end(); ++it )
                {
                    const Weak<Component> &w_component = *it;
                    const ptrdiff_t        idx         = std::distance( m_cached_component_.begin(), it );

                    if ( const Strong<Component> &component = w_component.lock() )
                    {
                        if ( component->m_ui_info_.dialogOpened )
                        {
                            if ( UIContext context = IUIAPI::NewContext( ui.NewDialog(
                                    this,
                                    std::format( "ComponentDialog{}", idx ),
                                    { component->m_ui_info_.label, component->m_ui_info_.dialogOpened } ) ) )
                            {
                                component->OnUIUpdate( &context, dt );
                            }
                        }
                    }
                }

                if ( m_b_child_dialog_ )
                {
                    if ( UIContext child_context = ui.NewContext( ui.NewDialog( this,
                        "ChildDialog",
                        { m_ui_info_.temporaryStrings[ "child_title" ], m_b_child_dialog_ } ) ) )
                    {
                        ( child_context |= ui.NewButton( this, "AddChildButton", { "Add Child" } ) ).SetFunction( [&]()
                        {
                            m_b_child_add_dialog_ = !m_b_child_add_dialog_;
                        } );

                        if ( m_b_child_add_dialog_ )
                        {
                            if ( UIContext child_select_context = IUIAPI::NewContext(
                                    ui.NewDialog( this,
                                                  "AddNewChildButton",
                                                  { "Add New Child", m_b_child_add_dialog_ } ) ) )
                            {
                                ( child_select_context |= ui.NewButton( this, "AddObjectButton", { "Object" } ) ).
                                        SetFunction( [this]()
                                        {
                                            if ( const Strong<Scene> &scene = GetScene().lock() )
                                            {
                                                const Strong<Object> &child = scene->CreateGameObject<Object>(
                                                        GetLayer() ).lock();
                                                AddChild( child, false );
                                            }

                                            m_b_child_add_dialog_ = false;
                                        } );

                                for ( const auto &[ type, predicate ] : ObjectFactory::GetGenerators() )
                                {
                                    ( child_select_context |= ui.NewButton(
                                            this,
                                            std::format( "Add{}Button", type->GetTypeName() ),
                                            { type->GetTypeName() } ) ).SetFunction( [this, &predicate]()
                                    {
                                        if ( const Strong<Scene> &scene = GetScene().lock() )
                                        {
                                            const Strong<ObjectBase> &child = predicate();
                                            scene->AddGameObject( GetLayer(), child );
                                            AddChild( child, false );
                                        }

                                        m_b_child_add_dialog_ = false;
                                    } );
                                }
                            }
                        }

                        child_context += ui.NewListBox( this, "ChildListBox", { "Children", 0, 0 } );
                        child_context |= ui.NewDragAndDropTarget( this,
                                                                  "DragAndDropTarget",
                                                                  { "OBJECT",
                                                                    [ this ]( void *ptr )
                                                                    {
                                                                        if ( const Strong<ObjectBase> *scary_ptr =
                                                                                static_cast<Strong<ObjectBase> *>( ptr ) )
                                                                        {
                                                                            if ( ( *scary_ptr )->GetParent().lock() ==
                                                                                GetSharedPtr<ObjectBase>() )
                                                                            {
                                                                                return;
                                                                            }

                                                                            if ( const Strong<Scene> &scene = GetScene()
                                                                                    .lock() )
                                                                            {
                                                                                scene->ChangeLayer(
                                                                                        GetLayer(),
                                                                                        ( *scary_ptr )->GetID() );
                                                                                AddChild( ( *scary_ptr ) );
                                                                            }
                                                                        }
                                                                    } } );

                        if ( m_children_cache_.empty() )
                        {
                            child_context |= ui.NewText( this, "EmptyChildText", { "No child found." } );
                        }

                        for ( auto it = m_children_cache_.begin(); it != m_children_cache_.end(); ++it )
                        {
                            const auto &[ id, child ] = *it;

                            if ( const Strong<ObjectBase> &locked = child.lock() )
                            {
                                ( child_context |= ui.NewButton( this,
                                                                 std::format( "Remove{}Button", id ),
                                                                 { "Remove" } ) ).SetFunction( [this, &locked]()
                                {
                                    if ( const Strong<Scene> &scene = locked->GetScene().lock() )
                                    {
                                        DetachChild( locked->GetLocalID() );
                                        scene->RemoveGameObject( locked->GetID(), locked->GetLayer() );
                                    }
                                } );
                                child_context |= ui.NewSameLine( this, std::format( "SL{}", id ), {} );
                                child_context |= ui.NewSelectable( this,
                                                                   std::format( "ChildSelectable{}", id ),
                                                                   { locked->m_ui_info_.label,
                                                                     locked->m_ui_info_.dialogOpened } );
                                child_context |= ui.NewDragAndDropSource( this,
                                                                          std::format( "ChildDragAndDropSource{}", id ),
                                                                          { "OBJECT",
                                                                            locked->m_ui_info_.label,
                                                                            &locked,
                                                                            sizeof( decltype( locked ) ) } );

                                if ( locked->m_ui_info_.dialogOpened )
                                {
                                    locked->OnUIUpdate( &child_context, dt );
                                }
                            }
                        }

                        --child_context;
                    }
                }
			}
		}
	}
#endif

	void ObjectBase::PreUpdate(const float dt)
	{
		for (const auto& component : m_components_ | std::views::values)
		{
			if (!component->GetActive())
			{
				continue;
			}

			component->PreUpdate(dt);
		}

		for (const auto& child : m_children_cache_ | std::views::values)
		{
			if (const auto locked = child.lock())
			{
				if (!locked->GetActive())
				{
					continue;
				}

				locked->PreUpdate(dt);
			}
		}
	}

	void ObjectBase::PreRender(const float dt)
	{
		for (const auto& child : m_children_cache_ | std::views::values)
		{
			if (const auto locked = child.lock())
			{
				if (!locked->GetActive())
				{
					continue;
				}

				locked->PreRender(dt);
			}
		}
	}

	void ObjectBase::Update(const float dt)
	{
		for (const auto& component : m_components_ | std::views::values)
		{
			if (!component->GetActive())
			{
				continue;
			}

			component->Update(dt);
		}

		for (const auto& child : m_children_cache_ | std::views::values)
		{
			if (const auto locked = child.lock())
			{
				if (!locked->GetActive())
				{
					continue;
				}

				locked->Update(dt);
			}
		}
	}
} // namespace Engine::Abstract
