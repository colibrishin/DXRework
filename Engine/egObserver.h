#pragma once
#include "egCommon.hpp"
#include "egObject.hpp"

namespace Engine::Objects
{
    class Observer : public Abstract::Object
    {
    public:
        OBJECT_T(DEF_OBJ_T_OBSERVER)

        Observer();
        void Initialize() override;
        ~Observer() override;
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void OnImGui() override;
        void OnDeserialized() override;

    private:
        SERIALIZER_ACCESS
    };
} // namespace Engine::Objects

BOOST_CLASS_EXPORT_KEY(Engine::Objects::Observer)
