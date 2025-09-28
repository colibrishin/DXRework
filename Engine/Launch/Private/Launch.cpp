#ifdef CFG_MONOLITH
#include "Monolith/Monolith.h"
#else
#include "WinAPIWrapper.hpp"
#include "EngineEntryPoint.h"
#include "ModuleInfo.h"
#include "ModuleManager.h"
#endif

#if _WIN32 || _WIN64
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow )
{
#ifndef CFG_MONOLITH
    const auto& load_seq =
            []( Engine::ModuleInfo& module, const std::wstring& filename, const std::filesystem::path& entry )
    {
        module.m_filename_     = entry.stem();
        module.m_filename_ext_ = filename;
        module.m_path_         = entry;

        if ( const HMODULE hModule = GetModuleHandleW( entry.c_str() ) )
        {
            module.m_handle_    = hModule;
            module.m_b_dynamic_ = false;
        }
        else
        {
            module.m_handle_    = LoadLibraryW( entry.c_str() );
            module.m_b_dynamic_ = true;
        }

        if ( !module.m_handle_ )
        {
            module.m_last_error_ = GetLastError();
            throw std::runtime_error( "Unable to load the essential library" );
        }

        using ModuleInitializationFunctionCStyle = Engine::IModule* ( * )();

        if ( const ModuleInitializationFunctionCStyle& init_func = ( ModuleInitializationFunctionCStyle )GetProcAddress(
                     static_cast<HMODULE>( module.m_handle_ ), "InitializeModule" ) )
        {
            module.m_module_ = std::unique_ptr<Engine::IModule>( init_func() );
            module.m_module_->Initialize();
        }
    };

    const auto& cleanup_seq = []( Engine::ModuleInfo& module )
    {
        if( module.m_module_ && GetModuleHandleW( module.m_path_.c_str() ) )
        {
            module.m_module_->Shutdown();
            module.m_module_.reset();

            if ( module.m_handle_ )
            {
                FreeLibrary( static_cast<HMODULE>( module.m_handle_ ) );
            }
            else
            {
                module.m_handle_ = nullptr;
            }
        }
    };

    // Exception guard
    try
    {
        std::filesystem::path              graphics_module;

        for ( const auto& entry : std::filesystem::directory_iterator( "./" ) )
        {
            if ( const std::wstring& file_name = entry.path().stem().generic_wstring();
                 entry.is_regular_file() && entry.path().extension() == ".dll" )
            {
                if ( file_name.ends_with( L"GraphicInterface" ) )
                {
                    graphics_module = entry;
                }
            }
        }

        // Memory and type management module
        g_core_mem = std::make_unique<Engine::ModuleInfo>();
        load_seq( *g_core_mem, L"Memory.dll", "./Memory.dll" );

        // Load core modules and graphic API, and OS API wrapper.
        g_module_api = std::make_unique<Engine::ModuleInfo>();
        load_seq( *g_module_api, L"CoreModule.dll", "./CoreModule.dll" );

        g_core_api = std::make_unique<Engine::ModuleInfo>();
        load_seq( *g_core_api, L"Core.dll", "./Core.dll" );

        g_os_api = std::make_unique<Engine::ModuleInfo>();
        load_seq( *g_os_api, L"WinAPIWrapper.dll", "./WinAPIWrapper.dll" );

        WinAPI::WinAPIWrapper::Initialize( hInstance );

        g_graphic_api = std::make_unique<Engine::ModuleInfo>();
        load_seq( *g_graphic_api, graphics_module.filename(), graphics_module );

        Engine::Managers::EngineEntryPoint::GetInstance().Initialize();
        WinAPI::WinAPIWrapper::Update();
    }
    catch ( std::exception& e )
    {
        // todo: alert
    }

    // Clean up core libraries
    cleanup_seq( *g_core_api );

    if ( Engine::Managers::EngineEntryPoint::IsInitialized() )
    {
        Engine::Managers::EngineEntryPoint::Destroy();
        Engine::Managers::ModuleManager::GetInstance().Shutdown();
    }

    // Clean up the graphic API.
    if ( g_graphic_api )
    {
        cleanup_seq( *g_graphic_api );
        g_graphic_api.reset();
    }

    // Kill the WinAPI wrapper
    if ( g_os_api )
    {
        cleanup_seq( *g_os_api );
        g_os_api.reset();
    }

    for ( Engine::alloc_base* alloc : g_static_alloc | std::views::values )
    {
        alloc->purge_memory();

        auto& rebind_releases = alloc->get_rebind_release();
        auto& rebind_purge    = alloc->get_rebind_purge();

        for ( const auto& [type, func] : rebind_purge )
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

    cleanup_seq( *g_module_api );

    if ( g_core_mem )
    {
        cleanup_seq( *g_core_mem );
        g_core_mem.reset();
    }

#else
    MonolithicLaunch( hInstance );
#endif
    return 0;
}
#endif