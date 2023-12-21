#include "pch.h"
#include "clPlayerModel.h"
#include <egResourceManager.hpp>

SERIALIZER_ACCESS_IMPL(
                       Client::Models::PlayerModel,
                       _ARTAG(_BSTSUPER(Engine::Resources::Model)))

namespace Client::Models
{
    PlayerModel::PlayerModel() : Model("./player.obj") {}
}
