#pragma once
#include "Texture2D.h"

#include "IntensityTexture.generated.h"

ECLASS(resource=client, serialize)
class ENGINE_CLIENT_API IntensityTexture : public Engine::Resources::Texture2D
{
    GENERATE_BODY
    IntensityTexture(const std::filesystem::path& path);

private:
    IntensityTexture();
};
