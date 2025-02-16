#pragma once
#include <filesystem>
#include <functional>

#include "boost/preprocessor/facilities/is_empty.hpp"
#include "Source/Runtime/CoreSingleton/Public/Singleton.h"

#include "ModuleManager.generated.h"

#define IS_DLL !BOOST_PP_IS_EMPTY(ENGINE_CORE_API)

namespace Engine
{
    struct IModule;
}

namespace Engine::Managers
{
	class ModuleManager;
}

namespace Engine::Managers
{
    using ModuleInitializationFunctionCStyle = IModule *( * )();
    using ModuleInitializationFunction       = std::function<IModule *()>;

    ECLASS()
    class ModuleManager : public Abstracts::Singleton<ModuleManager>
    {
        GENERATE_BODY
    public:
        ENGINE_CORE_API explicit ModuleManager( SINGLETON_LOCK_TOKEN );

    private:
        struct ModuleInfo
        {
            std::wstring          m_filename_ext_;
            std::wstring          m_filename_;
            std::filesystem::path m_path_;

            void    *m_handle_;
            bool     m_b_dynamic_  = false;
            uint64_t m_last_error_ = 0;

            std::unique_ptr<IModule> m_module_;
        };

        using ModuleInfoPtr = std::unique_ptr<ModuleInfo>;
        using ModuleMap     = std::unordered_map<std::wstring, ModuleInfoPtr>;

        void TryResolveLazyness( const std::wstring_view name );

    public:
        ENGINE_CORE_API void        Initialize() override;
        ENGINE_CORE_API ModuleInfo *FindModule( const std::wstring_view name );
        ENGINE_CORE_API IModule    *LoadModule( const std::wstring_view name );
        ENGINE_CORE_API void        AddModule( const std::wstring_view name );
#if IS_DLL
        ENGINE_CORE_API void RemoveModule( const std::wstring_view name );
#endif
        ENGINE_CORE_API void LoadModuleAll();

#if !IS_DLL
        void RegisterStaticModule( const std::wstring_view name, const ModuleInitializationFunction &func );
#endif

    private:
        ModuleManager() = default;
        friend struct SingletonDeleter;
        ~ModuleManager() override;

        void PreUpdate( const float dt ) override;
        void FixedUpdate( const float dt ) override;
        void Update( const float dt ) override;
        void PreRender( const float dt ) override;
        void Render( const float dt ) override;
        void PostRender( const float dt ) override;
        void PostUpdate( const float dt ) override;

        std::recursive_mutex                                           m_read_mutex_;
        std::recursive_mutex                                           m_write_mutex_;
        std::unordered_map<std::wstring, ModuleInfoPtr>                m_module_loaded_{};
        std::unordered_map<std::wstring, ModuleInitializationFunction> m_module_initializer_{};
        std::unordered_map<std::wstring, std::filesystem::path>        m_module_paths_{};
        std::unordered_map<std::wstring, std::set<std::wstring>>       m_lazy_modules_{};
#if !IS_DLL
        template <typename ModuleType>
        struct StaticLinkModuleEntry
        {
            explicit StaticLinkModuleEntry( const std::wstring_view name )
            {
                ModuleManager::RegisterStaticModule( name, &StaticLinkModuleEntry<ModuleType>::InitializeModule );
            }

            static IModule *InitializeModule()
            {
                return new ModuleType();
            }
        }
#endif
    };
}