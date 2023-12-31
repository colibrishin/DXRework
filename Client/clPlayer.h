#pragma once
#include <egObject.hpp>

namespace Client::Object
{
    class Player : public Engine::Abstract::Object
    {
    public:
        void Initialize() override;
        ~Player() override = default;

        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;
        void FixedUpdate(const float& dt) override;

    private:
        SERIALIZER_ACCESS
        std::map<UINT, LocalComponentID> m_child_bones_;

    };
}

BOOST_CLASS_EXPORT_KEY(Client::Object::Player)