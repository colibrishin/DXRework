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
        INTERNAL_OBJECT_CHECK_CONSTEXPR(DEF_OBJ_T_NONE)

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
                const auto thisObject = GetSharedPtr<Object>();

                boost::shared_ptr<T> component =
                        boost::make_shared<T>(thisObject, std::forward<Args>(args)...);
                component->Initialize();

                m_components_[which_component<T>::value].insert(
                                                       boost::reinterpret_pointer_cast<Component>(component));
                m_priority_sorted_.insert(
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

                return component;
            }

            return {};
        }

        template <typename T>
        boost::weak_ptr<T> GetComponent()
        {
            if constexpr (std::is_base_of_v<Component, T>)
            {
                if (!m_components_.contains(which_component<T>::value))
                {
                    return {};
                }

                const auto& comp_set = m_components_[which_component<T>::value];

                return boost::reinterpret_pointer_cast<T>(*comp_set.begin());
            }

            return {};
        }

        template <typename T>
        boost::weak_ptr<T> GetComponentByLocal(LocalComponentID id)
        {
            if (m_assigned_component_ids_.contains(id))
            {
                const auto& comp_set = m_components_[which_component<T>::value];
                const auto it = std::ranges::find(
                                                  comp_set, id, [](const auto& comp)
                                                  {
                                                      return comp->GetLocalID();
                                                  });

                if (it != comp_set.end())
                {
                    return boost::reinterpret_pointer_cast<T>(*it);
                }
            }

            return {};
        }

        const std::set<WeakComponent, ComponentPriorityComparer>& GetAllComponents() const;

        template <typename T>
        std::set<boost::weak_ptr<T>, WeakComparer<T>> GetComponents()
        {
            if constexpr (std::is_base_of_v<Component, T>)
            {
                if (!m_components_.contains(which_component<T>::value))
                {
                    return {};
                }

                const auto& comp_set = m_components_[which_component<T>::value];

                std::set<boost::weak_ptr<T>, WeakComparer<T>> result;

                for (const auto& comp : comp_set)
                {
                    result.insert(boost::reinterpret_pointer_cast<T>(comp));
                }

                return result;
            }

            return {};
        }

        template <typename T>
        boost::weak_ptr<T> GetComponent(GlobalEntityID id)
        {
            if constexpr (std::is_base_of_v<Component, T>)
            {
                if (!m_components_.contains(which_component<T>::value))
                {
                    return {};
                }

                const auto& comp_set = m_components_[which_component<T>::value];

                const auto found = std::find_if(
                                                comp_set.begin(), comp_set.end(),
                                                [id](const auto& comp)
                                                {
                                                    return comp.lock()->GetID() == id;
                                                });

                if (found == comp_set.end())
                {
                    return {};
                }

                return boost::reinterpret_pointer_cast<T>(*found);
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
                    const auto& comp_set = m_components_[which_component<T>::value];

                    if (comp_set.empty())
                    {
                        return;
                    }

                    const auto first = *comp_set.begin();
                    RemoveComponentCommon<T>(first);
                }
            }
        }

        template <typename T>
        void RemoveComponent(const LocalComponentID id)
        {
            if constexpr (std::is_base_of_v<Component, T>)
            {
                if (m_components_.contains(which_component<T>::value))
                {
                    const auto& comp_set = m_components_[which_component<T>::value];

                    if (comp_set.empty())
                    {
                        return;
                    }

                    const auto found = std::find_if(
                                                    comp_set.begin(), comp_set.end(),
                                                    [id](const auto& comp)
                                                    {
                                                        return comp->GetID() == id;
                                                    });

                    if (found == comp_set.end())
                    {
                        return;
                    }

                    RemoveComponentCommon<T>(*found);
                }
            }
        }

        template <typename T>
        void DispatchComponentEvent(boost::shared_ptr<T>& lhs, boost::shared_ptr<T>& rhs);

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
          m_parent_(g_invalid_id),
          m_type_(type),
          m_active_(true),
          m_culled_(true),
          m_imgui_open_(false) { };

    private:
        SERIALIZER_ACCESS
        friend class Scene;
        friend class Manager::Graphics::ShadowManager;

        template <typename T>
        void RemoveComponentCommon(const StrongComponent value)
        {
            m_priority_sorted_.erase(value);
            m_components_[which_component<T>::value].erase(value);
            m_assigned_component_ids_.erase(value->GetLocalID());

            if (const auto scene = GetScene().lock())
            {
                scene->RemoveCacheComponent<T>(value->GetSharedPtr<T>());
            }

            if (m_components_[which_component<T>::value].empty())
            {
                m_components_.erase(which_component<T>::value);
            }
        }

        virtual void OnCollisionEnter(const StrongCollider& other);
        virtual void OnCollisionContinue(const StrongCollider& other);
        virtual void OnCollisionExit(const StrongCollider& other);

    private:
        LocalActorID              m_parent_;
        std::vector<LocalActorID> m_children_;
        eDefObjectType       m_type_;
        bool                 m_active_ = true;
        bool                 m_culled_ = true;

        bool m_imgui_open_ = false;

        std::map<const eComponentType, std::set<StrongComponent, ComponentPriorityComparer>>
        m_components_;

        // Non-serialized
        std::map<LocalActorID, WeakObject>                      m_children_cache_;
        std::set<LocalComponentID>                              m_assigned_component_ids_;
        std::set<WeakComponent, ComponentPriorityComparer> m_priority_sorted_;
    };
} // namespace Engine::Abstract

BOOST_CLASS_EXPORT_KEY(Engine::Abstract::Object)
