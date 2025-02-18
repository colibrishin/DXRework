#pragma once
#include "Resource/Public/Resource.h"
#include "Prefab.generated.h"

namespace Engine::Resources
{
    ECLASS( resource, serialize )
    class ENGINE_CORE_API Prefab : public Abstracts::Resource
    {
        GENERATE_BODY
    public:
        void PreUpdate( const float dt ) override;
        void Update( const float dt ) override;
        void PostUpdate( const float dt ) override;
        void FixedUpdate( const float dt ) override;
        void OnDeserialized() override;
        void OnSerialized() override;

        // Add prefab object to the scene.
        void ExtractObject( const Strong<Scene> &scene, LayerSizeType layer ) const;

    protected:
        void Load_INTERNAL() override;
        void Unload_INTERNAL() override;

    private:
        explicit Prefab( const Strong<Abstracts::ObjectBase> &object );
        Prefab();

        Strong<Abstracts::ObjectBase> m_object_{};
        std::vector<Strong<Prefab> >  m_children_{};
    };
} // namespace Engine::Resources
