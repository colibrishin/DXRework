#pragma once
#include <boost/smart_ptr/enable_shared_from.hpp>

#include "egCommon.hpp"

namespace Engine::Abstract
{
    class Entity : public boost::enable_shared_from_this<Entity>
    {
    public:
        Entity(const Entity& other) = default;
        virtual ~Entity()           = default;

        bool operator==(const Entity& other) const
        {
            return GetID() == other.GetID();
        }

        void SetName(const EntityName& name);
        void SetGarbage(bool garbage);

        GlobalEntityID GetID() const;
        EntityName     GetName() const;
        TypeName       GetTypeName() const;
        bool           IsGarbage() const;

        template <typename T>
        __forceinline boost::weak_ptr<T> GetWeakPtr()
        {
            return boost::reinterpret_pointer_cast<T>(shared_from_this());
        }

        template <typename T>
        __forceinline boost::shared_ptr<T> GetSharedPtr()
        {
            return boost::reinterpret_pointer_cast<T>(shared_from_this());
        }

        virtual void Initialize();
        virtual void PreUpdate(const float& dt) = 0;
        virtual void Update(const float& dt) = 0;
        virtual void PostUpdate(const float& dt) = 0;
        virtual void FixedUpdate(const float& dt) = 0;

        virtual void OnDeserialized();
        virtual void OnImGui();

    protected:
        Entity()
        : m_b_initialized_(false),
          m_b_garbage_(false) {}

    private:
        SERIALIZER_ACCESS

        EntityName m_name_;
        bool       m_b_initialized_;
        bool       m_b_garbage_;
    };
} // namespace Engine::Abstract

BOOST_CLASS_EXPORT_KEY(Engine::Abstract::Entity)
