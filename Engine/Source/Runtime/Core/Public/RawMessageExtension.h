#pragma once
#undef GetMessage
#include "TypeLibrary.h"
#include "INetworkAPI.h"
#include "CoreType.h"

namespace Engine 
{
    struct ENGINE_CORE_API RawMessageExtension
    {
        Unique<NetMessage>&& GetMessage( const RawNetMessage& msg ) const;
    };
}