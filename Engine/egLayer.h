#pragma once
#include "egCommon.hpp"
#include "egRenderable.h"

namespace Engine
{
    class Layer final : public Abstract::Renderable
    {
    public:
        Layer(eLayerType type);

        ~Layer() override = default;

        void Initialize() override;
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PostUpdate(const float& dt) override;
        void OnDeserialized() override;

        void                           AddGameObject(const StrongObject& obj);
        void                           RemoveGameObject(EntityID id);
        WeakObject                     GetGameObject(EntityID id) const;
        const std::vector<WeakObject>& GetGameObjects();

        auto begin() noexcept { return m_objects_.begin(); }
        auto end() noexcept { return m_objects_.end(); }
        auto begin() const noexcept { return m_objects_.begin(); }
        auto end() const noexcept { return m_objects_.end(); }
        auto cbegin() const noexcept { return m_objects_.cbegin(); }
        auto cend() const noexcept { return m_objects_.cend(); }

    private:
        Layer();

        SERIALIZER_ACCESS

        eLayerType             m_layer_type_;
        std::set<StrongObject> m_objects_;

        // Non-serialized
        std::vector<WeakObject>              m_weak_objects_; // back-compatibility
        std::map<const EntityID, WeakObject> m_weak_objects_cache_;
    };
} // namespace Engine

BOOST_CLASS_EXPORT_KEY(Engine::Layer)
