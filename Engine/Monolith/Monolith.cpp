#include "Monolith.h"

#include "CoreModule.h"
#include "D3D12GraphicInterface.h"
#include "WinAPIWrapper.hpp"
#include "EngineEntryPoint.h"
#include "ModuleInfo.h"
#include "ModuleManager.h"

void MonolithicLaunch( HINSTANCE hInstance )
{
    try
    {
        WinAPI::WinAPIWrapper::Initialize( hInstance );

        g_graphic_api = std::make_unique<Engine::ModuleInfo>();

#if defined( USE_D3D12 )
        g_graphic_api->m_module_ = std::unique_ptr<Engine::IModule>(
                StaticLinkModuleEntry<Engine::D3D12GraphicInterfaceModule>::InitializeModule() );
#endif

        if ( g_graphic_api->m_module_ )
        {
            g_graphic_api->m_module_->Initialize();
        }

        Engine::Managers::EngineEntryPoint::GetInstance().Initialize();
        WinAPI::WinAPIWrapper::Update();
    }
    catch ( std::exception& e )
    {
        // todo: alert
    }

    if ( Engine::Managers::EngineEntryPoint::IsInitialized() )
    {
        Engine::Managers::EngineEntryPoint::Destroy();
    }

    // Also when startup failed before EngineEntryPoint existed: kept the same as Launch; static builds register no
    // shutdown callbacks. With no managed module loaded it does nothing else.
    Engine::Managers::ModuleManager::GetInstance().Shutdown();

    // Clean up the graphic API.
    if ( g_graphic_api )
    {
        if ( g_graphic_api->m_module_ )
        {
            g_graphic_api->m_module_->Shutdown();
        }
        g_graphic_api.reset();
    }

    // Container pools (g_static_alloc) are not emptied here: containers using them can still be alive (globals,
    // statics destroyed after this point), and pools that received memory from another DLL's copy of the same pool
    // have inconsistent free lists, so release_memory/purge_memory crashed or never returned when closing early.
    // The process is exiting; the OS reclaims the memory.
    g_allocator_storage.cleanup();
    PoolAllocatorStorage::report_leakage();
    Engine::Managers::ModuleManager::GetInstance().Destroy();
}
