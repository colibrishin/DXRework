#pragma once
#include "egActor.h"
#include "egCommon.hpp"
#include "egResource.h"
#include "egScene.hpp"
#include "egComponent.h"

namespace Engine::Abstract
{
    class Object : public Actor
    {
    public:
        OBJECT_T(DEF_OBJ_T_NONE)

        ~Object() override = default;

        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PostUpdate(const float& dt) override;
        void OnDeserialized() override;
        void OnImGui() override;

        template <typename T, typename... Args>
        boost::weak_ptr<T> AddComponent(Args&&... args)
        {
            if constexpr (std::is_base_of_v<Component, T>)
            {
                if (m_components_.contains(which_component<T>::value))
                {
                    return {};
                }

                const auto thisObject = GetSharedPtr<Object>();

                boost::shared_ptr<T> component =
                        boost::make_shared<T>(thisObject, std::forward<Args>(args)...);
                component->Initialize();

                m_components_.emplace(
                    which_component<T>::value, 
                    boost::reinterpret_pointer_cast<Component>(component));

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

                if (const auto scene = GetScene().lock())
                {
                    scene->AddCacheComponent<T>(component);
                }

                m_cached_component_.insert(component);

                return component;
            }

            return {};
        }

        const std::set<WeakComponent, ComponentPriorityComparer>& GetAllComponents();

        template <typename T>
        boost::weak_ptr<T> GetComponent()
        {
            if constexpr (std::is_base_of_v<Component, T>)
            {
                if (!m_components_.contains(which_component<T>::value))
                {
                    return {};
                }

                const auto& comp = m_components_[which_component<T>::value];

                return boost::static_pointer_cast<T>(comp);
            }

            return {};
        }

        template <typename T>
        void RemoveComponent()
        {
            if constexpr (std::is_base_of_v<Component, T>)
            {
                if (m_components_.contains(which_component<T>::value))
                {
                    const auto comp = m_components_[which_component<T>::value];

                    if (const auto scene = GetScene().lock())
                    {
                        scene->RemoveCacheComponent<T>(comp->template GetSharedPtr<T>());
                    }

                    if (m_components_[which_component<T>::value].empty())
                    {
                        m_components_.erase(which_component<T>::value);
                    }

                    m_assigned_component_ids_.erase(comp->GetLocalID());
                    m_cached_component_.erase(comp);
                    m_components_[which_component<T>::value].erase(comp);
                }
            }
        }

        template <typename T, typename Lock = std::enable_if_t<std::is_base_of_v<Component, T>>>
        void DispatchComponentEvent(const boost::shared_ptr<T>& other);

        void SetActive(bool active);
        void SetCulled(bool culled);
        void SetImGuiOpen(bool open);

        bool GetActive() const;
        bool GetCulled() const;
        bool GetImGuiOpen() const;
        eDefObjectType GetObjectType() const;

        WeakObject GetParent() const;
        WeakObject GetChild(const LocalActorID id) const;
        std::vector<WeakObject> GetChildren() const;

        void AddChild(const WeakObject& child);
        bool DetachChild(const LocalActorID id);

    protected:
        explicit Object(eDefObjectType type = DEF_OBJ_T_NONE)
        : Actor(),
          m_parent_id_(g_invalid_id),
          m_type_(type),
          m_active_(true),
          m_culled_(true),
          m_imgui_open_(false) { };

    private:
        SERIALIZER_ACCESS
        friend class Scene;
        friend class Manager::Graphics::ShadowManager;

        virtual void OnCollisionEnter(const StrongCollider& other);
        virtual void OnCollisionContinue(const StrongCollider& other);
        virtual void OnCollisionExit(const StrongCollider& other);

    private:
        LocalActorID              m_parent_id_;
        std::vector<LocalActorID> m_children_;
        eDefObjectType       m_type_;
        bool                 m_active_ = true;
        bool                 m_culled_ = true;

        bool m_imgui_open_ = false;

        std::map<eComponentType, StrongComponent> m_components_;

        // Non-serialized
        WeakObject                                         m_parent_;
        std::map<LocalActorID, WeakObject>                 m_children_cache_;
        std::set<LocalComponentID>                         m_assigned_component_ids_;
        std::set<WeakComponent, ComponentPriorityComparer> m_cached_component_;

    };
} // namespace Engine::Abstract

BOOST_CLASS_EXPORT_KEY(Engine::Abstract::Object)
