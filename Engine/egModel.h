#pragma once
#include "egResource.h"
#include "egMesh.h"

namespace Engine::Resources
{
	class Model : public Abstract::Resource
	{
    public:
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;

    protected:
        void Load_INTERNAL() override;
        void Unload_INTERNAL() override;

	private:
        Mesh m_mesh_;
        std::vector<Texture> m_textures_;
    };
}