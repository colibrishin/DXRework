#include "pch.h"
#include "egNormalMap.h"
#include "egRenderPipeline.h"

SERIALIZER_ACCESS_IMPL(
                       Engine::Resources::NormalMap,
                       _ARTAG(_BSTSUPER(Engine::Resources::Texture)))

namespace Engine::Resources
{
    NormalMap::NormalMap(std::filesystem::path path): Texture(std::move(path)) {}

    NormalMap::NormalMap(): Texture({}) {}

    NormalMap::~NormalMap() = default;


    void NormalMap::Initialize()
    {
        Texture::Initialize();
    }

    void NormalMap::PreUpdate(const float& dt)
    {
        Texture::PreUpdate(dt);
    }

    void NormalMap::Update(const float& dt)
    {
        Texture::Update(dt);
    }

    void NormalMap::PreRender(const float& dt)
    {
        Texture::PreRender(dt);
    }

    void NormalMap::Render(const float& dt)
    {
        GetRenderPipeline().BindResource(SR_NORMAL_MAP, m_texture_view_.Get());
    }

    void NormalMap::PostRender(const float& dt)
    {
        Texture::PostRender(dt);
    }

    void NormalMap::Load_INTERNAL()
    {
        Texture::Load_INTERNAL();
    }

    void NormalMap::Unload_INTERNAL()
    {
        Texture::Unload_INTERNAL();
    }

    eResourceType NormalMap::GetResourceType() const
    {
        return rtype;
    }
} // namespace Engine::Resources
