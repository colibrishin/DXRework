#pragma once
#include "Resource.h"
#include "ResourceManager.h"
#include "IGraphicAPI.h"

#include "Font.generated.h"

namespace Engine::Resources
{
    ECLASS(resource, serialize)
    class ENGINE_FONT_API Font : public Abstracts::Resource
    {
        GENERATE_BODY

        using Resource::Resource;

        Font(const Font&);
        Font& operator=(const Font&);

        void Initialize() override;
        void PreUpdate(const float dt) override;
        void Update(const float dt) override;
        void PostUpdate(const float dt) override;
        void FixedUpdate(const float dt) override;
        void OnSerialized() override;
        void OnDeserialized() override;
        IFont& GetPrimitive() const;

    private:
        friend struct ConstructorAccess;
        Font() : Resource("") {}
        void Load_INTERNAL() override;
        void Unload_INTERNAL() override;

        Unique<IFont> m_primitive_;
    };
}