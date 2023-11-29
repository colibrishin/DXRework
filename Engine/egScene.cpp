#include "pch.hpp"
#include "egScene.hpp"

#include "egManagerHelper.hpp"

namespace Engine
{
	void Scene::Initialize()
	{
		for(int i = 0; i < LAYER_MAX; ++i)
		{
			m_layers.emplace(static_cast<eLayerType>(i), Instantiate<Layer>(static_cast<eLayerType>(i)));
		}

		const auto camera = Instantiate<Objects::Camera>(GetSharedPtr<Scene>());
		AddGameObject(camera, LAYER_CAMERA);

		m_mainCamera_ = camera;

		const auto light1 = Instantiate<Objects::Light>(GetSharedPtr<Scene>());
		AddGameObject(light1, LAYER_LIGHT);
		light1->SetPosition(Vector3(5.0f, 5.0f, 5.0f));

		const auto light2 = Instantiate<Objects::Light>(GetSharedPtr<Scene>());
		light2->SetPosition(Vector3(-5.0f, 5.0f, -5.0f));
		light2->SetColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
		AddGameObject(light2, LAYER_LIGHT);
	}

	EntityID Scene::AddGameObject(const StrongObject& obj, eLayerType layer)
	{
		m_layers[layer]->AddGameObject(obj);
		m_cached_objects_.emplace(obj->GetID(), obj);

		if (const auto tr = obj->GetComponent<Component::Transform>().lock())
		{
			UpdatePosition(obj);
		}

		return obj->GetID();
	}

	void Scene::RemoveGameObject(const EntityID id, eLayerType layer)
	{
		const auto obj = m_cached_objects_[id];

		if (const auto locked = obj.lock())
		{
			const auto tr = locked->GetComponent<Component::Transform>().lock();
			bool updated = false;

			if (tr)
			{
				const auto prev_pos = tr->GetPreviousPosition() + Vector3::One * g_octree_negative_round_up;
				const auto pos = tr->GetPosition() + Vector3::One * g_octree_negative_round_up;

				auto& prev_pos_set = m_object_position_tree_(static_cast<int>(prev_pos.x), static_cast<int>(prev_pos.y), static_cast<int>(prev_pos.z));
				auto& pos_set = m_object_position_tree_(static_cast<int>(pos.x), static_cast<int>(pos.y), static_cast<int>(pos.z));

				if (prev_pos_set.contains(obj))
				{
					prev_pos_set.erase(obj);
					updated = true;
				}
				if (pos_set.contains(obj))
				{
					pos_set.erase(obj);
					updated = true;
				}
			}

			if (!updated)
			{
				// @todo: add task for refreshing octree.
			}
		}

		m_cached_objects_.erase(id);
		m_layers[layer]->RemoveGameObject(id);
	}

	void Scene::UpdatePosition(const WeakObject& obj)
	{
		if (const auto obj_ptr = obj.lock())
		{
			const auto tr = obj_ptr->GetComponent<Component::Transform>().lock();

			if (!tr)
			{
				GetDebugger().Log(L"Object has no transform component");
				return;
			}

			const auto prev_pos = VectorElementAdd(tr->GetPreviousPosition(), g_octree_negative_round_up);
			const auto pos = VectorElementAdd(tr->GetPosition(), g_octree_negative_round_up);

			if (!VectorElementInRange(prev_pos, g_max_map_size) || !VectorElementInRange(pos, g_max_map_size))
			{
				GetDebugger().Log(L"Object position is out of range");
				return;
			}

			const auto delta = prev_pos - pos;

			if (std::fabsf(delta.x) <= g_epsilon && 
				std::fabsf(delta.y) <= g_epsilon &&
				std::fabsf(delta.z) <= g_epsilon)
			{
				return;
			}

			auto& prev_pos_set = m_object_position_tree_(static_cast<int>(prev_pos.x), static_cast<int>(prev_pos.y), static_cast<int>(prev_pos.z));
			auto& pos_set = m_object_position_tree_(static_cast<int>(pos.x), static_cast<int>(pos.y), static_cast<int>(pos.z));

			if (prev_pos_set.contains(obj))
			{
				prev_pos_set.erase(obj);
			}

			if (!pos_set.contains(obj))
			{
				pos_set.insert(obj);
			}
		}
	}

	void Scene::GetNearestObjects(const Vector3& pos, std::vector<WeakObject>& out)
	{
		const auto pos_rounded = VectorElementAdd(pos, g_octree_negative_round_up);

		if (!VectorElementInRange(pos_rounded, g_max_map_size))
		{
			GetDebugger().Log(L"Position is out of range");
			return;
		}

		const auto& pos_set = m_object_position_tree_(static_cast<int>(pos_rounded.x), static_cast<int>(pos_rounded.y), static_cast<int>(pos_rounded.z));

		for (const auto& obj : pos_set)
		{
			if (const auto obj_ptr = obj.lock())
			{
				out.push_back(obj_ptr);
			}
		}
	}

	void Scene::GetNearbyObjects(const Vector3& pos, const size_t range, std::vector<WeakObject>& out)
	{
		const auto pos_rounded = VectorElementAdd(pos, g_octree_negative_round_up);

		for (auto i = static_cast<size_t>(pos_rounded.x) - range; i < static_cast<size_t>(pos_rounded.x) + range; ++i)
		{
			for (auto j = static_cast<size_t>(pos_rounded.y) - range; j < static_cast<size_t>(pos_rounded.y) + range; ++j)
			{
				for (auto k = static_cast<size_t>(pos_rounded.z) - range; k < static_cast<size_t>(pos_rounded.z) + range; ++k)
				{
					if (!VectorElementInRange(pos_rounded, g_max_map_size))
					{
						GetDebugger().Log(L"Position is out of range");
						continue;
					}

					const auto& set = m_object_position_tree_(i, j, k);

					for (const auto& obj : set)
					{
						if (const auto obj_ptr = obj.lock())
						{
							out.push_back(obj_ptr);
						}
					}
				}
			}
		}
	}

	void Scene::SearchObjects(const Vector3& pos, const Vector3& dir, std::set<WeakObject, WeakObjComparer>& out, int exhaust)
	{
		auto pos_rounded = VectorElementAdd(pos, g_octree_negative_round_up);
		float accumulated_length = 0.f;

		while (static_cast<int>(accumulated_length) < exhaust)
		{
			const auto& current_tree = m_object_position_tree_(static_cast<int>(pos_rounded.x), static_cast<int>(pos_rounded.y), static_cast<int>(pos_rounded.z));

			for (const auto& obj : current_tree)
			{
				if (const auto obj_ptr = obj.lock())
				{
					out.insert(obj_ptr);
				}
			}

			pos_rounded += dir;
			accumulated_length += dir.Length();
		}
	}
}
