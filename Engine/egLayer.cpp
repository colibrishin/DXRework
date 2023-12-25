#include "pch.h"
#include "egLayer.h"
#include "egObject.hpp"

SERIALIZER_ACCESS_IMPL(
                       Engine::Layer,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Renderable))
                       _ARTAG(m_layer_type_) _ARTAG(m_objects_))

namespace Engine
{
    Layer::Layer(eLayerType type): m_layer_type_(type) { }

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

    void Layer::OnDeserialized()
    {
        Renderable::OnDeserialized();

        // rebuild cache
        for (const auto& object : m_objects_)
        {
            object->OnDeserialized();
            m_weak_objects_cache_[object->GetID()] = object;
            m_weak_objects_.emplace_back(object);
        }
    }

    void Layer::AddGameObject(const StrongObject& obj)
    {
        if (m_objects_.contains(obj))
        {
            return;
        }

        m_objects_.insert(obj);
        m_weak_objects_cache_[obj->GetID()] = obj;
        m_weak_objects_.emplace_back(obj);
    }

    void Layer::RemoveGameObject(EntityID id)
    {
        if (m_weak_objects_cache_.contains(id))
        {
            m_objects_.erase(m_weak_objects_cache_[id].lock());
            m_weak_objects_cache_.erase(id);
            std::erase_if(
                          m_weak_objects_,
                          [id](const auto& obj)
                          {
                              return obj.lock()->GetID() == id;
                          });
        }
    }

    WeakObject Layer::GetGameObject(EntityID id) const
    {
        for (const auto& object : m_objects_)
        {
            if (object->GetID() == id)
            {
                return object;
            }
        }

        return {};
    }

    const std::vector<WeakObject>& Layer::GetGameObjects()
    {
        return m_weak_objects_;
    }

    Layer::Layer(): m_layer_type_(LAYER_NONE) {}
} // namespace Engine
