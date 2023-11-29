#include "pch.hpp"
#include "egActor.hpp"

#include "egManagerHelper.hpp"

namespace Engine::Abstract
{
	void Actor::SetLayer(eLayerType layer)
	{
		const auto current_scene = m_assigned_scene_.lock();

		if (!current_scene)
		{
			return;
		}

		Manager::TaskScheduler::GetInstance().AddTask([this, layer](const float& dt)
		{
			OnLayerChanging();
			m_assigned_scene_.lock()->RemoveGameObject(GetID(), m_layer_);
			m_layer_ = layer;
			m_assigned_scene_.lock()->AddGameObject(GetSharedPtr<Object>(), m_layer_);
			OnLayerChanged();
		});
	}

	void Actor::SetScene(EntityID scene_id)
	{
		const auto next_scene = Manager::SceneManager::GetInstance().GetScene(scene_id);
		const auto current_scene = m_assigned_scene_.lock();

		if (!current_scene)
		{
			return;
		}

		if (const auto locked = next_scene.lock())
		{
			Manager::TaskScheduler::GetInstance().AddTask([this, scene_id, next_scene](const float& dt)
			{
				m_assigned_scene_.lock()->RemoveGameObject(GetID(), m_layer_);
				m_assigned_scene_ = next_scene;
				next_scene.lock()->AddGameObject(GetSharedPtr<Object>(), m_layer_);
			});
		}
	}
}
