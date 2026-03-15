#pragma once

#include <filesystem>
#include <functional>
#include <set>
#include <unordered_map>
#include <string>
#include <mutex>

#include "CoreType.h"
#include "IModule.h"
#include "ModuleInfo.h"

#include "ModuleManager.generated.h"

#if IS_DLL
extern ENGINE_COREMODULE_API std::unique_ptr<Engine::ModuleInfo> g_os_api;
extern ENGINE_COREMODULE_API std::unique_ptr<Engine::ModuleInfo> g_core_mem;
extern ENGINE_COREMODULE_API std::unique_ptr<Engine::ModuleInfo> g_module_api;
extern ENGINE_COREMODULE_API std::unique_ptr<Engine::ModuleInfo> g_core_api;
#endif

extern ENGINE_COREMODULE_API std::unique_ptr<Engine::ModuleInfo> g_graphic_api;

#if !IS_DLL
#define MODULE_IMPL( ModuleType, Name )                                                                                \
    static StaticLinkModuleEntry<ModuleType>        ModuleEntry##Name(WIDEN(STRINGIFY(Name)));                         \
    extern "C" void                          MODULE_IMPL_##Name()                                                      \
    {}
#else
#define MODULE_IMPL( ModuleType, Name )                                                                                \
    extern "C" DLLEXPORT Engine::IModule *InitializeModule()                                                           \
    {                                                                                                                  \
        return new ModuleType();                                                                                       \
    }                                                                                                                  \
    extern "C" void MODULE_IMPL_##Name()                                                                               \
    {}
#endif

namespace Engine::Managers
{
    using ModuleInitializationFunctionCStyle = Engine::IModule *( * )();
    using ModuleInitializationFunction       = std::function<Engine::IModule *()>;

    struct ENGINE_COREMODULE_API ModuleManager final
    {
    private:
        using ModuleInfoPtr = std::unique_ptr<ModuleInfo>;
        using ModuleMap     = std::unordered_map<std::wstring, ModuleInfoPtr>;

        void TryResolveLazyness( const std::wstring_view name );
#if IS_DLL
        bool CheckNoInit( const ModuleInfo* module_info );
#else
        bool CheckNoInit( const std::wstring_view module_name );
#endif
        /// Creates IModule* without Initialize; used by LoadModuleAll for phase-aware sort.
        bool CreateModuleOnly( const std::wstring_view name );
        /// Initializes an already-created module and appends to load order.
        void InitializeModuleInOrder( const std::wstring_view name );
        /// Returns names sorted by ELoadPhase then by dependency (topological order).
        std::vector<std::wstring> SortByPhaseAndDependency( const std::vector<std::wstring>& names );
    public:
        ModuleManager() = default;
        ~ModuleManager();

        void                  Initialize();
        void                  Shutdown();
        void                  Destroy();
        ModuleInfo*           FindModule( const std::wstring_view name );
        IModule*              LoadModule( const std::wstring_view name );
        void                  AddModule( const std::wstring_view name );
        void                  ShutdownModule( const std::wstring_view name );
        void                  LoadModuleAll();
        static ModuleManager& GetInstance();

        /// Callback invoked for each registered listener when a module is about to shut down (before
        /// IModule::Shutdown). Use so subsystems can unregister all state owned by that module.
        using OnModuleShutdownCallback = std::function<void( std::wstring_view )>;
        void RegisterOnModuleShutdown( OnModuleShutdownCallback cb );

#if !IS_DLL
        void RegisterStaticModule( const std::wstring_view name, const ModuleInitializationFunction& func );
#endif

    private:
        std::recursive_mutex                                     m_read_mutex_;
        std::recursive_mutex                                     m_write_mutex_;
        std::unordered_map<std::wstring, ModuleInfoPtr>          m_module_loaded_;
        std::unordered_map<std::wstring, std::filesystem::path>  m_module_paths_;
        std::unordered_map<std::wstring, std::set<std::wstring>> m_lazy_modules_;
        std::list<std::wstring>                                  m_module_load_order_;
        std::vector<OnModuleShutdownCallback>                    m_on_module_shutdown_;

#if !IS_DLL
        std::unordered_map<std::wstring, ModuleInitializationFunction> m_module_initializer_;
#endif
    };
}

#if !IS_DLL
template <typename ModuleType>
struct StaticLinkModuleEntry
{
    explicit StaticLinkModuleEntry( const std::wstring_view name )
    {
        Engine::Managers::ModuleManager::GetInstance().RegisterStaticModule( name, &InitializeModule );
    }

    static Engine::IModule* InitializeModule()
    {
        return new ModuleType();
    }
};
#endif
