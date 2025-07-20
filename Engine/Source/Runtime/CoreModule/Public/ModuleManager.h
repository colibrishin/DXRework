#pragma once

#include <filesystem>
#include <functional>
#include <set>
#include <unordered_map>
#include <string>
#include <mutex>

#include "CoreType.h"

extern ENGINE_COREMODULE_API std::unique_ptr<Engine::ModuleInfo> g_os_api;
extern ENGINE_COREMODULE_API std::unique_ptr<Engine::ModuleInfo> g_graphic_api;
extern ENGINE_COREMODULE_API std::vector<std::unique_ptr<Engine::ModuleInfo>> g_core_api;

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

namespace Engine
{
    struct ModuleInfo;
    struct IModule;
}

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

    public:
        ModuleManager() = default;
        ~ModuleManager();

        void                  Initialize();
        void                  Destroy();
        ModuleInfo*           FindModule( const std::wstring_view name );
        IModule*              LoadModule( const std::wstring_view name );
        void                  AddModule( const std::wstring_view name );
        void                  RemoveModule( const std::wstring_view name );
        void                  LoadModuleAll();
        static ModuleManager& GetInstance();

#if !IS_DLL
        void RegisterStaticModule( const std::wstring_view name, const ModuleInitializationFunction& func );
#endif

    private:
        std::recursive_mutex                                     m_read_mutex_;
        std::recursive_mutex                                     m_write_mutex_;
        std::unordered_map<std::wstring, ModuleInfoPtr>          m_module_loaded_;
        std::unordered_map<std::wstring, std::filesystem::path>  m_module_paths_;
        std::unordered_map<std::wstring, std::set<std::wstring>> m_lazy_modules_;

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
