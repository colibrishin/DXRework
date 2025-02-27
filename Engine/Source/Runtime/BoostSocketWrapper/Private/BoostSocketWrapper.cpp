#include "BoostSocketWrapper.h"
#include "BoostSocketWrapper.generated.h"

void Engine::BoostSocketWrapper::Initialize()
{

}

void Engine::BoostSocketWrapper::Shutdown()
{

}

bool Engine::BoostSocketWrapper::Listen( const unsigned short port )
{
    return false;
}

bool Engine::BoostSocketWrapper::Connect( const std::string_view ip, const unsigned short port )
{
    return false;
}

void Engine::BoostSocketWrapper::Send( const RawNetMessage& message )
{
    
}

Engine::NetworkMessageTask& Engine::BoostSocketWrapper::GetMessageTask()
{
    return m_message_task_;
}
