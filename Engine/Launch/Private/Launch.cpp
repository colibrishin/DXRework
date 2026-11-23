#ifdef CFG_MONOLITH
#include "Monolith/Monolith.h"
#else
#if PLATFORM == Windows
#include <Windows.h>
#endif
#include "WinAPIWrapper.hpp"
#include "EngineEntryPoint.h"
#include "ModuleInfo.h"
#include "ModuleManager.h"
#include "ModuleRegistration.h"
#endif

#if PLATFORM == Windows
// Both launch paths (editor and monolithic client/server); WIN32_LEAN_AND_MEAN keeps Windows.h from including it.
#include <objbase.h>
#pragma comment( lib, "ole32.lib" )
#endif

#if PLATFORM == Windows
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow )
{
    // Texture loading (WIC) needs COM on this thread. Without this, COM was only initialized when the window was
    // activated (the text input service does it on focus), so a launch that did not get focus failed to load
    // textures with E_NOINTERFACE.
    const HRESULT com_result = CoInitializeEx( nullptr, COINIT_APARTMENTTHREADED );

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

            // Only release what LoadLibraryW referenced (GetModuleHandleW takes no reference).
            if ( module.m_handle_ && module.m_b_dynamic_ )
            {
                FreeLibrary( static_cast<HMODULE>( module.m_handle_ ) );
            }

            module.m_handle_ = nullptr;
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

        // Core (manual) load order follows kCoreModuleLoadOrder (ELoadPhase::Core).
        static_assert( kCoreModuleLoadOrder[0] == L"Memory" && kCoreModuleLoadOrder[3] == L"WinAPIWrapper" );

        g_core_mem = std::make_unique<Engine::ModuleInfo>();
        load_seq( *g_core_mem, L"Memory.dll", "./Memory.dll" );

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
#if PLATFORM == Windows
        MessageBoxA( nullptr, e.what(), "Launch Exception", MB_OK | MB_ICONERROR );
#endif
    }

    // Teardown order per ModuleRegistration.h: Destroy → Shutdown → Core → Graphic → OS → alloc → Destroy → CoreModule → Memory
    if ( Engine::Managers::EngineEntryPoint::IsInitialized() )
    {
        Engine::Managers::EngineEntryPoint::Destroy();
    }

    // Also when startup failed before EngineEntryPoint existed: Core registered its shutdown callback at load, and
    // Shutdown() is what destroys the callbacks. With no managed module loaded it does nothing else.
    Engine::Managers::ModuleManager::GetInstance().Shutdown();

    if ( g_core_api )
    {
        cleanup_seq( *g_core_api );
    }

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

    // Container pools (g_static_alloc) are not emptied here: containers using them can still be alive (globals,
    // statics destroyed after this point), and pools that received memory from another DLL's copy of the same pool
    // have inconsistent free lists, so release_memory/purge_memory crashed or never returned when closing early.
    // The process is exiting; the OS reclaims the memory.
    g_allocator_storage.cleanup();
    PoolAllocatorStorage::report_leakage();
    Engine::Managers::ModuleManager::GetInstance().Destroy();

    if ( g_module_api )
    {
        cleanup_seq( *g_module_api );
    }

    if ( g_core_mem )
    {
        cleanup_seq( *g_core_mem );
        g_core_mem.reset();
    }

#else
    MonolithicLaunch( hInstance );
#endif

    // Every COM user (graphics and sound modules, the window) is torn down above.
    if ( SUCCEEDED( com_result ) )
    {
        CoUninitialize();
    }
    return 0;
}
#endif