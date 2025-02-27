#pragma once
#include "INetworkAPI.h"
#include "NetworkMessageTask.h"
#include "BoostSocketWrapper.generated.h"

namespace Engine
{
    ECLASS(virtual)
    struct ENGINE_BOOSTSOCKETWRAPPER_API BoostSocketWrapper : public INetworkAPI
    {
        GENERATE_BODY
        void Initialize() override;
        void Shutdown() override;
        bool Listen( const unsigned short port ) override;
        bool Connect( const std::string_view ip, const unsigned short port ) override;
        void Send( const RawNetMessage& message ) override;
        NetworkMessageTask& GetMessageTask() override;

    private:
        NetworkMessageTask m_message_task_;
    };
}