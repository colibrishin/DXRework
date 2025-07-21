#include "ModuleManager.h"

#include <iostream>
#include <ranges>

#include "ModuleInfo.h"
#include "IModule.h"

#if _WIN32 || _WIN64
#include <Windows.h>
#endif

std::unique_ptr<Engine::ModuleInfo>              g_os_api      = nullptr;
std::unique_ptr<Engine::ModuleInfo>              g_graphic_api = nullptr;
std::unique_ptr<Engine::ModuleInfo>              g_core_mem    = nullptr;
std::vector<std::unique_ptr<Engine::ModuleInfo>> g_core_api    = {};

namespace Engine::Managers
{
	void ModuleManager::TryResolveLazyness(const std::wstring_view name)
	{
        for ( auto it = m_lazy_modules_.begin(); it != m_lazy_modules_.end(); )
		{
			if ( it->second.contains( name.data() ) )
			{
                it->second.erase( name.data() );

				if ( it->second.empty() )
				{
                    std::wstring load_finished = it->first;
                    m_lazy_modules_.erase( it );
                    LoadModule( load_finished );
                    it = m_lazy_modules_.begin();
                    continue;
				}
			}

			++it;
		}
    }

    bool ModuleManager::CheckNoInit( const ModuleInfo* module_info )
    {
        // Ignore load/unload self
        if ( module_info->m_filename_ == L"CoreModule" )
        {
            return true;
        }

        // OS API handle is loaded before the module manager.
        if ( module_info->m_handle_ == g_os_api->m_handle_ )
        {
            return true;
        }

        // Graphic API is loaded before the module manager.
        if ( module_info->m_handle_ == g_graphic_api->m_handle_ )
        {
            return true;
        }

        // Memory and type management moudle should be loaded before the module manager.
        if ( module_info->m_handle_ == g_core_mem->m_handle_ )
        {
            return true;
        }

        // Core libraries are loaded before the module manager.
        if ( std::ranges::find_if( g_core_api,
                                   [ &module_info ]( const std::unique_ptr<ModuleInfo>& elem )
                                   { return module_info->m_handle_ == elem->m_handle_; } ) != g_core_api.end() )
        {
            return true;
        }

        return false;
    }

	void ModuleManager::Initialize()
	{
		m_module_paths_.emplace(L"Default", "./");
    }

    void ModuleManager::Destroy()
    {
#if IS_DLL
        const auto& resolve = [ this, resolve ]( const std::wstring_view module_name, ModuleInfo& info )
        {
            if ( GetModuleHandleW( info.m_path_.c_str() ) && info.m_module_ )
            {
                bool                            found      = false;
                const std::vector<std::string>& dependency = info.m_module_->GetDependencies();

                for ( const std::string& name : dependency )
                {
                    std::wstring conversion( name.begin(), name.end() );

                    if ( m_module_loaded_.contains( conversion ) )
                    {
                        m_lazy_modules_[ conversion ].insert( module_name.data() );
                        found = true;
                    }
                }

                if ( !found )
                {
                    info.m_module_->Shutdown();
                    info.m_module_.reset();

                    if ( info.m_handle_ )
                    {
                        FreeLibrary( static_cast<HMODULE>( info.m_handle_ ) );
                    }

                    return true;
                }

                return false;
            }
        };
#endif

        for ( auto it = m_module_loaded_.begin(); it != m_module_loaded_.end(); )
        {
            auto& ptr = it->second;
#if IS_DLL
            if ( resolve( it->first, *it->second ) )
            {
                if ( m_lazy_modules_.contains( it->first ) )
                {
                    // todo: 
                }

                it = m_module_loaded_.erase( it );
            }
#else
            if ( ptr->m_module_ )
            {
                ptr->m_module_.reset();
            }

            if ( ptr )
            {
                ptr.reset();
            }

            it = m_module_loaded_.erase( it );
#endif
        }
	}

	ModuleInfo* ModuleManager::FindModule(const std::wstring_view name)
	{
		std::lock_guard l(m_write_mutex_);

		if (!m_module_loaded_.contains(name.data()))
		{
			return nullptr;
		}

		return m_module_loaded_.at( name.data() ).get();
	}

	Engine::IModule* ModuleManager::LoadModule(const std::wstring_view name)
	{
		ModuleInfo* module_info = FindModule(name);

		if (module_info)
		{
			if (IModule* module = module_info->m_module_.get())
			{
                if ( module_info->m_b_lazy )
                {
                    module_info->m_module_->Initialize();
                    module_info->m_b_lazy = false;
                    TryResolveLazyness( name );
                }

				return module;
			}
		}
		else 
		{
			AddModule(name);
			module_info = FindModule(name);
		}

		{
            std::lock_guard l( m_read_mutex_ );
#if !IS_DLL
            // Static Library
            if ( m_module_initializer_.contains( name.data() ) )
            {
                if ( const ModuleInitializationFunction &func = m_module_initializer_.at( name.data() ) )
                {
                    module_info->m_module_ = std::unique_ptr<IModule>( func() );

                    if ( module_info->m_module_ )
                    {
                        for ( const std::string_view required : module_info->m_module_->LoadAfter() )
                        {
                            std::wstring conversion( required.begin(), required.end() );

                            if ( !FindModule( conversion ) )
                            {
                                m_lazy_modules_[ name.data() ].insert( conversion );
                            }
                        }

                        for ( const std::string_view dependency : module_info->m_module_->GetDependencies() )
                        {
                            std::wstring conversion( dependency.begin(), dependency.end() );

                            if ( !FindModule( conversion ) )
                            {
                                m_lazy_modules_[ name.data() ].insert( conversion );
                            }
                        }

                        if ( m_lazy_modules_.contains( name.data() ) )
                        {
                            RemoveModule( name );
                            return nullptr;
                        }
                    }

					std::string conversion( name.begin(), name.end() );
					CONSOLE_OUT( "ModuleManager", "Module {} loaded", conversion.c_str() )

                    module_info->m_module_->Initialize();
                    TryResolveLazyness( name );
                    return module_info->m_module_.get();
                }
                else
                {
                    return nullptr;
                }
            }
#else
            // DLL
            if ( const HMODULE hModule = GetModuleHandleW( module_info->m_path_.c_str() ) )
            {
                module_info->m_handle_    = hModule;
                module_info->m_b_dynamic_ = false;
            }
            else
            {
                module_info->m_handle_    = LoadLibraryW( module_info->m_path_.c_str() );
                module_info->m_b_dynamic_ = true;
            }

            if ( !module_info->m_handle_ )
            {
                module_info->m_last_error_ = GetLastError();
                return nullptr;
            }

            if ( !CheckNoInit(module_info) )
            {
                const ModuleInitializationFunctionCStyle& init_func =
                        ( ModuleInitializationFunctionCStyle )GetProcAddress(
                                static_cast<HMODULE>( module_info->m_handle_ ), "InitializeModule" );

                if ( init_func )
                {
                    module_info->m_module_ = std::unique_ptr<IModule>( init_func() );
                    // Assuming that the dependent libraries are loaded.
                    module_info->m_b_lazy  = false;

                    if ( module_info->m_module_ )
                    {
                        for ( const std::string_view required : module_info->m_module_->LoadAfter() )
                        {
                            std::wstring conversion( required.begin(), required.end() );

                            if ( !FindModule( conversion ) )
                            {
                                m_lazy_modules_[ name.data() ].insert( conversion );
                                module_info->m_b_lazy = true;
                            }
                        }

                        for ( const std::string_view dependency : module_info->m_module_->GetDependencies() )
                        {
                            std::wstring conversion( dependency.begin(), dependency.end() );

                            if ( !FindModule( conversion ) )
                            {
                                m_lazy_modules_[ name.data() ].insert( conversion );
                                module_info->m_b_lazy = true;
                            }
                        }

                        if ( m_lazy_modules_.contains( name.data() ) )
                        {
                            module_info->m_b_lazy = true;
                            return nullptr;
                        }

                        
                        module_info->m_module_->Initialize();
                    }
                }

                // todo: dll whitelist
            }
#endif
            if ( !module_info->m_b_lazy )
            {
                CONSOLE_OUT( "ModuleManager", "Module {} loaded", name.data() );
                TryResolveLazyness( name );
                return module_info->m_module_.get();
            }

            return nullptr;
        }
	}

#if !IS_DLL
	void ModuleManager::RegisterStaticModule(const std::wstring_view name, const ModuleInitializationFunction& func)
	{
        std::lock_guard l( m_read_mutex_ );
		m_module_initializer_.emplace(name, func);
	}
#endif

	void ModuleManager::AddModule(const std::wstring_view name)
	{
		{
            std::lock_guard l( m_write_mutex_ );
			if (m_module_loaded_.contains(name.data()))
			{
				return;
			}
		}
		
		std::lock_guard l( m_read_mutex_ );
		m_module_loaded_.emplace(name, std::make_unique<ModuleInfo>());

		ModuleInfo* module_info = m_module_loaded_.at(name.data()).get();
		module_info->m_filename_ = name;

		bool found = false;
#if IS_DLL
		module_info->m_filename_ext_ = std::wstring(name) + L".dll";

		for (const auto& path : m_module_paths_ | std::views::values)
		{
			if (found)
			{
				break;
			}

			if (std::filesystem::is_directory(path))
			{
				for (const std::filesystem::directory_entry& it : std::filesystem::directory_iterator(path))
				{
					if (it.path().extension() == L".dll" && it.path().stem() == name)
					{
						module_info->m_path_ = std::filesystem::absolute(it.path());
						found = true;
						break;
					}
				}
			}
		}
#else
		found = true;
#endif
	}

	void ModuleManager::RemoveModule(const std::wstring_view name)
	{
		{
			std::lock_guard l(m_write_mutex_);
			if (!m_module_loaded_.contains(name.data()))
			{
				return;
			}
		}
		
		std::lock_guard l(m_read_mutex_);
		std::unique_ptr<ModuleInfo> module_info = std::move(m_module_loaded_.at(name.data()));
		
		if (module_info)
		{
			HMODULE module_ptr = static_cast<HMODULE>(module_info->m_handle_);
			module_info.reset(); // Free the module information first to avoid the incomplete type.
#if IS_DLL
			FreeLibrary(module_ptr); // Free the library
#endif
		}
		
		m_module_loaded_.erase(name.data());
    }

	void ModuleManager::LoadModuleAll()
	{
#if IS_DLL
		for (const auto& directory : m_module_paths_)
		{
			for (const auto& entry : std::filesystem::directory_iterator(directory.second))
			{
				if (const std::wstring& file_name = entry.path().stem().generic_wstring();
					entry.is_regular_file() && entry.path().extension() == ".dll")
				{
					LoadModule(file_name);
				}
			}
		}
#else
		for (const auto& [name, func] : m_module_initializer_)
		{
            LoadModule( name.data() );
		}
#endif
	}

    ModuleManager& ModuleManager::GetInstance()
    {
        static std::unique_ptr<ModuleManager> instance;

		if (!instance)
		{
            instance = std::make_unique<ModuleManager>();
		}

		return *instance;
	}

    ModuleManager::~ModuleManager() {}
} // namespace Engine::Managers
