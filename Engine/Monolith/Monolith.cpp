#include "Monolith.h"

#include "CoreModule.h"
#include "D3D12GraphicInterface.h"
#include "WinAPIWrapper.hpp"
#include "EngineEntryPoint.h"
#include "ModuleInfo.h"
#include "ModuleManager.h"

void MonolithicLaunch( HINSTANCE hInstance )
{
    WinAPI::WinAPIWrapper::Initialize( hInstance );

    g_graphic_api = std::make_unique<Engine::ModuleInfo>();

#if defined( USE_D3D12 )
    g_graphic_api->m_module_ = std::unique_ptr<Engine::IModule>(
            StaticLinkModuleEntry<Engine::D3D12GraphicInterfaceModule>::InitializeModule() );
#endif

    g_graphic_api->m_module_->Initialize();

    try
    {
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
        Engine::Managers::ModuleManager::GetInstance().Shutdown();
    }

    // Clean up the graphic API.
    if ( g_graphic_api )
    {
        g_graphic_api->m_module_->Shutdown();
        g_graphic_api.reset();
    }

    for ( Engine::alloc_base* alloc : g_static_alloc | std::views::values )
    {
        alloc->purge_memory();

        auto& rebind_releases = alloc->get_rebind_release();
        auto& rebind_purge    = alloc->get_rebind_purge();

        for ( const auto& [ type, func ] : rebind_purge )
        {
            func();
        }

        rebind_releases.clear();
        rebind_purge.clear();
    }

    g_static_alloc.clear();
    g_allocator_storage.cleanup();
    PoolAllocatorStorage::report_leakage();
    Engine::Managers::ModuleManager::GetInstance().Destroy();
}
