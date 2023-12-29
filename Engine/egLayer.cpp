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
            m_weak_objects_cache_.insert({object->GetID(), object});
        }
    }

    void Layer::AddGameObject(const StrongObject& obj)
    {
        if (m_objects_.contains(obj))
        {
            return;
        }

        m_objects_.insert(obj);
        m_weak_objects_cache_.insert({obj->GetID(), obj});
    }

    void Layer::RemoveGameObject(GlobalEntityID id)
    {
        ConcurrentWeakObjGlobalMap::const_accessor obj;

        if (m_weak_objects_cache_.find(obj, id))
        {
            m_objects_.erase(obj->second.lock());
            m_weak_objects_cache_.erase(id);
        }
    }

    WeakObject Layer::GetGameObject(GlobalEntityID id) const
    {
        ConcurrentWeakObjGlobalMap::const_accessor obj;

        if (m_weak_objects_cache_.find(obj, id))
        {
            return obj->second;
        }

        return {};
    }

    ConcurrentWeakObjVec Layer::GetGameObjects() const
    {
        ConcurrentWeakObjVec result;

        for (const auto& obj : m_weak_objects_cache_ | std::views::values)
        {
            result.push_back(obj);
        }

        return result;
    }

    Layer::Layer(): m_layer_type_(LAYER_NONE) {}
} // namespace Engine
