#include "UIInterface.h"

std::unique_ptr<Engine::UIInterface>  Engine::UIInterfaceAccessor::m_ui_interface_ = {};
std::set<Engine::UITokenInputContext> Engine::UIDialogMapper::m_reserved_          = {};

inline Engine::UITokenInputContext Engine::UIDialogMapper::Map( const void *ptr )
{
    Engine::UITokenInputContext value = reinterpret_cast<Engine::UITokenInputContext>( ptr );
    while (true)
    {
        if ( !m_reserved_.contains(value) )
        {
            break;
        }

        ++value;
    }

    m_reserved_.emplace( value );
    return value;
}

inline void Engine::UIDialogMapper::Unmap( Engine::UITokenInputContext value )
{
    if ( m_reserved_.contains( value ) )
    {
        m_reserved_.erase( value );
    }
}

inline void Engine::UIDialogMapper::Clear()
{
    m_reserved_.clear();
}