#include "ModuleManager.h"

#include <iostream>
#include <ranges>

#include "NetworkType.h"

#if Platform == Windows
#include <Windows.h>
#endif

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

	void ModuleManager::Initialize()
	{
		m_module_paths_.emplace(L"Default", "./");
    }

    ModuleManager::ModuleInfoPtr ModuleManager::Destroy()
    {
        ModuleInfoPtr core_ptr;
        ModuleInfoPtr api_ptr;
        ModuleInfoPtr gapi_ptr;

        for ( auto it = m_module_loaded_.begin(); it != m_module_loaded_.end(); )
        {
            auto& ptr = it->second;

            if ( ptr->m_filename_.starts_with( L"Core" ) )
            {
                core_ptr = std::move( ptr );
                it       = m_module_loaded_.erase( it );
                continue;
            }

            if ( ptr->m_filename_.ends_with( L"Wrapper" ) )
            {
                api_ptr = std::move( ptr );
                it      = m_module_loaded_.erase( it );
                continue;
            }

			if ( ptr->m_filename_.ends_with( L"GraphicInterface" ) )
            {
                gapi_ptr = std::move( ptr );
                it       = m_module_loaded_.erase( it );
                continue;
            }

#if IS_DLL
            if ( ptr->m_handle_ )
            {
                if ( GetModuleHandleW( it->second->m_path_.c_str() ) && ptr->m_module_ )
				{
                    ptr->m_module_->Shutdown();
                    ptr->m_module_.reset();   
				}
                FreeLibrary( static_cast<HMODULE>( ptr->m_handle_ ) );
            }
#else
            if ( ptr->m_module_ )
            {
                ptr->m_module_.reset();
            }
#endif

		    if ( ptr )
		    {
                ptr.reset();
		    }

			it = m_module_loaded_.erase( it );
        }

        g_allocator_storage.cleanup();
        g_allocator_storage.report_leakage();

        if ( gapi_ptr )
        {
#if IS_DLL
            if ( gapi_ptr->m_handle_ )
            {
                if ( GetModuleHandleW( gapi_ptr->m_path_.c_str() ) && gapi_ptr->m_module_ )
                {
                    gapi_ptr->m_module_->Shutdown();
                    gapi_ptr->m_module_.reset();
                }
                FreeLibrary( static_cast<HMODULE>( gapi_ptr->m_handle_ ) );
            }
#else
            if ( gapi_ptr->m_module_ )
            {
                gapi_ptr->m_module_.reset();
            }
#endif
        }

        return std::move( api_ptr );
	}

	ModuleManager::ModuleInfo* ModuleManager::FindModule(const std::wstring_view name)
	{
		std::lock_guard l(m_write_mutex_);

		if (!m_module_loaded_.contains(name.data()))
		{
			return nullptr;
		}

		return m_module_loaded_.at(name.data()).get();
	}

	Engine::IModule* ModuleManager::LoadModule(const std::wstring_view name)
	{
		ModuleInfo* module_info = FindModule(name);

		if (module_info)
		{
			if (IModule* module = module_info->m_module_.get())
			{
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

            const ModuleInitializationFunctionCStyle &init_func = ( ModuleInitializationFunctionCStyle )GetProcAddress(
                    static_cast<HMODULE>( module_info->m_handle_ ), "InitializeModule" );

            if ( init_func )
            {
                module_info->m_module_ = std::unique_ptr<IModule>( init_func() );

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

				CONSOLE_OUT( "ModuleManager", "Module {} loaded", name.data() )
                module_info->m_module_->Initialize();
                TryResolveLazyness( name );
                return module_info->m_module_.get();
            }
            else
            {
                FreeLibrary( static_cast<HMODULE>( module_info->m_handle_ ) );
                return nullptr;
            }
#endif
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
			std::lock_guard l(m_write_mutex_);
			if (m_module_loaded_.contains(name.data()))
			{
				return;
			}
		}
		
		std::lock_guard l(m_read_mutex_);
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

    ModuleManager::~ModuleManager()
	{
        Destroy();
	}
    ModuleManager::ModuleInfo::~ModuleInfo()
    {
#if IS_DLL
        if ( !GetModuleHandleW( m_path_.c_str() ) )
        {
            m_module_.release();
        }
#endif
    }
} // namespace Engine::Managers
