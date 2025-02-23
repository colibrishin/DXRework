#pragma once

#ifndef DLLIMPORT
#define DLLIMPORT __declspec( dllimport )
#endif

#ifndef DLLEXPORT
#define DLLEXPORT __declspec( dllexport )
#endif

#include <filesystem>
#include <functional>
#include <set>
#include <unordered_map>
#include <string>
#include <mutex>

#include "boost/preprocessor/facilities/is_empty.hpp"

#ifdef _UNICODE
#ifndef UNICODE
#define UNICODE
#endif
#endif

#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define STRINGIFY(X)      STRINGIFY_IMPL(X)
#define STRINGIFY_IMPL(X) #X
#define IS_DLL !BOOST_PP_IS_EMPTY( ENGINE_COREMODULEMANAGER_API )

namespace Engine
{
    struct ENGINE_COREMODULEMANAGER_API IModule
    {
        virtual ~IModule() = default;

        void Initialize()
        {
            if ( InitializeImpl() )
            {
                m_b_is_initialized_ = true;
            }
        }
        void Shutdown()
        {
            if ( ShutdownImpl() )
            {
                m_b_is_initialized_ = false;
            }
        }

        virtual bool InitializeImpl()  = 0;
        virtual bool ShutdownImpl()    = 0;
        virtual bool DynamicLoadable() = 0;

        virtual const std::vector<std::string> &GetDependencies() const
        {
            static const std::vector<std::string> empty = {};
            return empty;
        }

        virtual const std::vector<std::string> &LoadAfter() const
        {
            static std::vector<std::string> load_after = {};
            return load_after;
        }

    private:
        bool m_b_is_initialized_ = false;
    };
} // namespace Engine

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

    struct ENGINE_COREMODULEMANAGER_API ModuleManager final
    {
    private:
        struct ModuleInfo
        {
            std::wstring          m_filename_ext_;
            std::wstring          m_filename_;
            std::filesystem::path m_path_;

            void    *m_handle_     = nullptr;
            bool     m_b_dynamic_  = false;
            uint64_t m_last_error_ = 0;

            std::unique_ptr<IModule> m_module_;
        };

        using ModuleInfoPtr = std::unique_ptr<ModuleInfo>;
        using ModuleMap     = std::unordered_map<std::wstring, ModuleInfoPtr>;

        static void TryResolveLazyness( const std::wstring_view name );

    public:
        ModuleManager() = default;
        ~ModuleManager();

        static void        Initialize();
        static void        Destroy();
        static ModuleInfo *FindModule( const std::wstring_view name );
        static IModule    *LoadModule( const std::wstring_view name );
        static void        AddModule( const std::wstring_view name );
        static void        RemoveModule( const std::wstring_view name );
        static void        LoadModuleAll();

#if !IS_DLL
        static void RegisterStaticModule( const std::wstring_view name, const ModuleInitializationFunction &func );
#endif

    private:
        static std::recursive_mutex                                    m_read_mutex_;
        static std::recursive_mutex                                    m_write_mutex_;
        static std::unordered_map<std::wstring, ModuleInfoPtr>         m_module_loaded_;
        static std::unordered_map<std::wstring, std::filesystem::path> m_module_paths_;
        static std::unordered_map<std::wstring, std::set<std::wstring>> m_lazy_modules_;

#if !IS_DLL
        static std::unordered_map<std::wstring, ModuleInitializationFunction> m_module_initializer_;     
#endif
    };
}

template <typename ModuleType>
struct StaticLinkModuleEntry
{
    explicit StaticLinkModuleEntry( const std::wstring_view name )
    {
        Engine::Managers::ModuleManager::RegisterStaticModule( name,
                                                               &InitializeModule );
    }

    static Engine::IModule *InitializeModule()
    {
        return new ModuleType();
    }
};