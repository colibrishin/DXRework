#pragma once
#include <egModel.h>

namespace Client::Models
{
    class PlayerModel final : public Engine::Resources::Model
	{
    public:
        PlayerModel();

    private:
        SERIALIZER_ACCESS
	};
}

BOOST_CLASS_EXPORT_KEY(Client::Models::PlayerModel)