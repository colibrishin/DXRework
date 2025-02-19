#pragma once
#include "Texture2D.h"

#include "ShadowMaskTexture.generated.h"

ECLASS(resource=client, serialize)
class ENGINE_CLIENT_API ShadowMaskTexture : public Engine::Resources::Texture2D
{
    GENERATE_BODY
    ShadowMaskTexture( const std::filesystem::path &path );

private:
    ShadowMaskTexture();
};