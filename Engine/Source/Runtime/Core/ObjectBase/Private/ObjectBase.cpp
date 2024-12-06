#include "../Public/ObjectBase.hpp"

#include <any>

#include "Source/Runtime/Managers/TaskScheduler/Public/TaskScheduler.h"
#include "Source/Runtime/Core/Script/Public/Script.h"
#include "Source/Runtime/Core/Components/Collider/Public/Collider.hpp"

SERIALIZE_IMPL
(
 Engine::Abstracts::ObjectBase,
 _ARTAG(_BSTSUPER(Actor))
 _ARTAG(m_parent_id_)
 _ARTAG(m_children_)
 _ARTAG(m_type_)
 _ARTAG(m_active_)
 _ARTAG(m_culled_)
 _ARTAG(m_components_)
 _ARTAG(m_scripts_)
)

namespace Engine::Abstracts
{
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

	void ObjectBase::AddChild(const Weak<ObjectBase>& p_child, const bool immediate)
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

	void ObjectBase::OnCollisionEnter(const Strong<Components::Collider>& other)
	{
		if (!GetComponent<Components::Collider>().lock())
		{
			throw std::exception("Object has no collider");
		}

		for (const auto& script : m_scripts_ | std::views::values)
		{
			if (!script->GetActive())
			{
				continue;
			}

			script->OnCollisionEnter(other);
		}
	}

	void ObjectBase::OnCollisionContinue(const Strong<Components::Collider>& other)
	{
		if (!GetComponent<Components::Collider>().lock())
		{
			throw std::exception("Object has no collider");
		}

		for (const auto& script : m_scripts_ | std::views::values)
		{
			if (!script->GetActive())
			{
				continue;
			}

			script->OnCollisionContinue(other);
		}
	}

	void ObjectBase::OnCollisionExit(const Strong<Components::Collider>& other)
	{
		if (!GetComponent<Components::Collider>().lock())
		{
			throw std::exception("Object has no collider");
		}

		for (const auto& script : m_scripts_ | std::views::values)
		{
			if (!script->GetActive())
			{
				continue;
			}

			script->OnCollisionExit(other);
		}
	}

	Weak<Abstracts::Component> ObjectBase::checkComponent(const eComponentType type)
	{
		if (m_components_.contains(type))
		{
			return m_components_[type];
		}

		return {};
	}

	Weak<Script> ObjectBase::checkScript(const ScriptSizeType type)
	{
		if (m_scripts_.contains(type))
		{
			return m_scripts_[type];
		}

		return {};
	}

	void ObjectBase::removeScript(const ScriptSizeType type)
	{
		removeScriptFromSceneCache(m_scripts_[type]);
		removeScriptImpl(type);
	}

	void ObjectBase::removeScriptImpl(const ScriptSizeType type)
	{
		if (m_scripts_.contains(type))
		{
			Managers::TaskScheduler::GetInstance().AddTask
					(
					 TASK_REM_SCRIPT,
					 {GetSharedPtr<ObjectBase>(), type},
					 [](const std::vector<std::any>& params, const float)
					 {
						 const auto& obj  = std::any_cast<Strong<ObjectBase>>(params[0]);
						 const auto& type = std::any_cast<ScriptSizeType>(params[1]);

						 std::erase_if
								 (
								  obj->m_cached_script_, [type](const auto& script)
								  {
									  if (const auto& locked = script.lock())
									  {
										  return locked->GetScriptType() == type;
									  }

									  return false;
								  }
								 );
						 obj->m_scripts_.erase(type);
					 }
					);
		}
	}

	Weak<Abstracts::Component> ObjectBase::addComponent(const Strong<Component>& component)
	{
		const auto type = component->GetComponentType();

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

		// Add the component to the object.
		addComponentImpl(component, type);

		return component;
	}

	Weak<Script> ObjectBase::addScript(const Strong<Script>& script)
	{
		const auto type = script->GetScriptType();

		if (const auto scp = checkScript(type).lock())
		{
			return scp;
		}

		// Remove the component from the previous owner.
		if (const auto prev = script->GetOwner().lock();
			prev && prev != GetSharedPtr<ObjectBase>())
		{
			prev->removeScript(script->GetScriptType());
		}

		// Change the owner of the component. Since the component is already added to the cache, skipping the uncaching.
		script->SetOwner(GetSharedPtr<ObjectBase>());

		// Add the component to the object.
		addScriptImpl(script, type);
		addScriptToSceneCache(script);

		return script;
	}

	void ObjectBase::addScriptImpl(const Strong<Script>& script, const ScriptSizeType type)
	{
		script->SetOwner(GetSharedPtr<ObjectBase>());

		if (!script->IsInitialized())
		{
			script->Initialize();
		}

		m_scripts_.emplace(type, script);

		m_cached_script_.push_back(script);
	}

	void ObjectBase::removeComponentImpl(const eComponentType type, const Strong<Component>& comp)
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
					 const auto& type = std::any_cast<eComponentType>(params[2]);

					 obj->m_assigned_component_ids_.erase(comp->GetLocalID());
					 obj->m_cached_component_.erase(comp);
					 obj->m_components_.erase(type);
				 }
				);
	}

	void ObjectBase::removeComponent(const eComponentType type)
	{
		if (m_components_.contains(type))
		{
			const auto& comp = m_components_[type];

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

	void ObjectBase::addComponentImpl(const Strong<Component>& component, eComponentType type)
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
	}

	void ObjectBase::Render(const float& dt)
	{
		for (const auto& script : m_scripts_ | std::views::values)
		{
			if (!script->GetActive())
			{
				continue;
			}

			script->Render(dt);
		}

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

	void ObjectBase::PostRender(const float& dt)
	{
		for (const auto& script : m_scripts_ | std::views::values)
		{
			if (!script->GetActive())
			{
				continue;
			}

			script->PostRender(dt);
		}

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

	void ObjectBase::FixedUpdate(const float& dt)
	{
		for (const auto& script : m_scripts_ | std::views::values)
		{
			if (!script->GetActive())
			{
				continue;
			}

			script->FixedUpdate(dt);
		}

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

	void ObjectBase::PostUpdate(const float& dt)
	{
		for (const auto& script : m_scripts_ | std::views::values)
		{
			if (!script->GetActive())
			{
				continue;
			}

			script->PostUpdate(dt);
		}

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

		for (const auto& script : m_scripts_ | std::views::values)
		{
			script->OnSerialized();
		}
	}

	void ObjectBase::OnDeserialized()
	{
		Actor::OnDeserialized();

		for (const auto& comp : m_components_ | std::views::values)
		{
			comp->SetOwner(GetSharedPtr<ObjectBase>());
			comp->OnDeserialized();
			m_assigned_component_ids_.insert(comp->GetLocalID());
			m_cached_component_.insert(comp);
		}

		for (const auto& script : m_scripts_ | std::views::values)
		{
			script->SetOwner(GetSharedPtr<ObjectBase>());
			script->OnDeserialized();
			m_cached_script_.push_back(script);
		}
	}

	Strong<ObjectBase> ObjectBase::Clone(bool register_scene) const
	{
		const auto& cloned = cloneImpl();

		// Clone components and scripts
		cloned->m_components_.clear();
		cloned->m_scripts_.clear();

		// Erase the copied pointers that indicates original object.
		cloned->m_assigned_component_ids_.clear();
		cloned->m_cached_component_.clear();
		cloned->m_scripts_.clear();
		cloned->m_components_.clear();

		// Copy components
		for (const auto& comp : m_components_ | std::views::values)
		{
			const auto& cloned_comp = comp->Clone();
			cloned->addComponent(cloned_comp);
		}

		// Copy scripts
		for (const auto& script : m_scripts_ | std::views::values)
		{
			const auto& cloned_script = script->Clone(cloned);
			cloned->addScript(cloned_script);
		}

		// Keep intact with the parent.

		// Clone children
		cloned->m_children_.clear();
		cloned->m_children_cache_.clear();

		for (const auto& child : m_children_cache_ | std::views::values)
		{
			if (const auto locked = child.lock())
			{
				const auto cloned_child = locked->Clone(register_scene);
				cloned->AddChild(cloned_child);
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

	const std::vector<Weak<Script>>& ObjectBase::GetAllScripts()
	{
		return m_cached_script_;
	}

	void ObjectBase::PreUpdate(const float& dt)
	{
		for (const auto& script : m_scripts_ | std::views::values)
		{
			if (!script->GetActive())
			{
				continue;
			}

			script->PreUpdate(dt);
		}

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

	void ObjectBase::PreRender(const float& dt)
	{
		for (const auto& script : m_scripts_ | std::views::values)
		{
			if (!script->GetActive())
			{
				continue;
			}

			script->PreRender(dt);
		}

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

	void ObjectBase::Update(const float& dt)
	{
		for (const auto& script : m_scripts_ | std::views::values)
		{
			if (!script->GetActive())
			{
				continue;
			}

			script->Update(dt);
		}

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
