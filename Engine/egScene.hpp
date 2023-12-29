#pragma once
#include <ranges>

#include <boost/serialization/export.hpp>
#include "egRenderable.h"
#include "egLayer.h"
#include "../octree/octree.h"

namespace Engine
{
    class Scene : public Abstract::Renderable
    {
    public:
        Scene(const Scene& other) = default;
        ~Scene() override         = default;

        virtual void Initialize_INTERNAL() = 0;
        void         DisableControllers();
        void         Initialize() final;

        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void Save();
        void Render(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PostRender(const float& dt) override;
        void PostUpdate(const float& dt) override;

        void     OnDeserialized() override;

        template <typename T, typename ObjLock = std::enable_if_t<std::is_base_of_v<Abstract::Object, T>>>
        GlobalEntityID AddGameObject(const boost::shared_ptr<T>& obj, eLayerType layer)
        {
            m_layers[layer]->AddGameObject(obj);
            m_cached_objects_.emplace(obj->GetID(), obj);

            for (const auto& comp : obj->GetAllComponents())
            {
                ConcurrentWeakComTypeMap::accessor acc;

                m_cached_components_.insert(acc, comp.lock()->GetComponentType());
                acc->second.emplace(comp.lock()->GetID(), comp);
            }

            obj->template GetSharedPtr<Abstract::Actor>()->SetScene(GetSharedPtr<Scene>());
            obj->template GetSharedPtr<Abstract::Actor>()->SetLayer(layer);

            AssignLocalIDToObject(obj);

            if (const auto tr = obj->template GetComponent<Components::Transform>().lock())
            {
                UpdatePosition(obj);
            }

            if (layer == LAYER_LIGHT && !std::is_base_of_v<Objects::Light, T>)
            {
                static_assert("Only light object can be added to light layer");
            }
            else if (layer == LAYER_CAMERA && !std::is_base_of_v<Objects::Camera, T>)
            {
                static_assert("Only camera object can be added to camera layer");
            }

            if constexpr (std::is_base_of_v<Objects::Light, T>)
            {
                RegisterLightToManager(obj);
            }

            return obj->GetID();
        }

        void     RemoveGameObject(GlobalEntityID id, eLayerType layer);

        ConcurrentWeakObjVec GetGameObjects(eLayerType layer);

        WeakObject FindGameObject(GlobalEntityID id) const
        {
            ConcurrentWeakObjGlobalMap::const_accessor acc;

            if (m_cached_objects_.find(acc, id))
            {
                return acc->second;
            }

            return {};
        }

        WeakObject FindGameObjectByLocalID(LocalActorID id) const
        {
            ConcurrentLocalGlobalIDMap::const_accessor actor_acc;

            if (m_assigned_actor_ids_.find(actor_acc, id))
            {
                ConcurrentWeakObjGlobalMap::const_accessor acc;

                if (m_cached_objects_.find(acc, actor_acc->second))
                {
                    return acc->second;
                }
            }

            return {};
        }

        WeakCamera GetMainCamera() const
        {
            return m_mainCamera_;
        }

        auto begin() noexcept
        {
            return m_layers.begin();
        }

        auto end() noexcept
        {
            return m_layers.end();
        }

        auto begin() const noexcept
        {
            return m_layers.begin();
        }

        auto end() const noexcept
        {
            return m_layers.end();
        }

        auto cbegin() const noexcept
        {
            return m_layers.cbegin();
        }

        auto cend() const noexcept
        {
            return m_layers.cend();
        }

        template <typename T, typename CompLock = std::enable_if_t<std::is_base_of_v<Abstract::Component, T>>>
        void AddCacheComponent(const boost::shared_ptr<T>& component)
        {
            ConcurrentWeakObjGlobalMap::const_accessor acc;

            if (m_cached_objects_.find(acc, component->GetOwner().lock()->GetID()))
            {
                ConcurrentWeakComTypeMap::accessor comp_acc;
                if (m_cached_components_.find(comp_acc, which_component<T>::value))
                {
                    comp_acc->second.emplace(component->GetID(), component);
                }
            }
        }

        template <typename T, typename CompLock = std::enable_if_t<std::is_base_of_v<Abstract::Component, T>>>
        void RemoveCacheComponent(const boost::shared_ptr<T>& component)
        {
            ConcurrentWeakObjGlobalMap::const_accessor acc;

            if (m_cached_objects_.find(acc, component->GetOwner().lock()->GetID()))
            {
                ConcurrentWeakComTypeMap::accessor comp_acc;
                if (m_cached_components_.find(comp_acc, which_component<T>::value))
                {
                    comp_acc->second.erase(component->GetID());
                }
            }
        }

        template <typename T>
        ConcurrentWeakComVec GetCachedComponents()
        {
            ConcurrentWeakComTypeMap::const_accessor acc;

            if (m_cached_components_.find(acc, which_component<T>::value))
            {
                ConcurrentWeakComVec result;

                for (const auto& comp : acc->second | std::views::values)
                {
                    result.push_back(comp);
                }

                return result;
            }

            return {};
        }

        eSceneType GetType() const;

        void UpdatePosition(const WeakObject& obj);
        void GetNearestObjects(const Vector3& pos, std::vector<WeakObject>& out);
        void GetNearbyObjects(
            const Vector3 &           pos, const UINT range,
            std::vector<WeakObject> & out);
        void SearchObjects(
            const Vector3&                                        pos, const Vector3& dir,
            std::set<WeakObject, WeakComparer<Abstract::Object>>& out,
            int                                                   exhaust = 100);

    protected:
        Scene();

    private:
        SERIALIZER_ACCESS

        void Synchronize(const StrongScene& scene);
        void OpenLoadPopup(bool& is_load_open);

        void AssignLocalIDToObject(const StrongObject& obj);
        void RegisterLightToManager(const StrongLight & obj);
        void UnregisterLightFromManager(const StrongLight & obj);

        void RemoveObjectFromCache(const WeakObject& obj);
        void RemoveObjectFromOctree(const WeakObject& obj);

        virtual void AddCustomObject();

        LocalActorID                           m_main_camera_local_id_;
        std::map<eLayerType, StrongLayer> m_layers;
        eSceneType                        m_type_;

        // Non-serialized
        WeakObject m_observer_;
        WeakCamera m_mainCamera_;

        ConcurrentLocalGlobalIDMap m_assigned_actor_ids_;
        ConcurrentWeakObjGlobalMap m_cached_objects_;
        ConcurrentWeakComTypeMap m_cached_components_;
        Octree<std::set<WeakObject, WeakComparer<Abstract::Object>>>
        m_object_position_tree_;
    };
} // namespace Engine

BOOST_CLASS_EXPORT_KEY(Engine::Scene)
