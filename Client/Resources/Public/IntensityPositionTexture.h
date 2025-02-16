#pragma once
#include "Texture2D.h"

#include "IntensityPositionTexture.generated.h"

ECLASS(resource=client, serialize)
class ENGINE_CLIENT_API IntensityPositionTexture : public Engine::Resources::Texture2D
{
    GENERATE_BODY
    IntensityPositionTexture( const std::filesystem::path &path );

private:
    IntensityPositionTexture();
};
