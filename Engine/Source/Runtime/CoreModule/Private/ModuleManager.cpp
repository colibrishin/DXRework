#include "ModuleManager.h"

#include <algorithm>
#include <iostream>
#include <queue>
#include <ranges>

#include "ModuleInfo.h"
#include "IModule.h"
#include "ModuleRegistration.h"

#if PLATFORM == Windows
#include <Windows.h>
#endif

#if IS_DLL
std::unique_ptr<Engine::ModuleInfo>              g_os_api      = nullptr;
std::unique_ptr<Engine::ModuleInfo>              g_core_mem    = nullptr;
std::unique_ptr<Engine::ModuleInfo>              g_module_api  = nullptr;
std::unique_ptr<Engine::ModuleInfo>              g_core_api    = nullptr;
#endif

std::unique_ptr<Engine::ModuleInfo> g_graphic_api = nullptr;

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

#if !IS_DLL
    bool ModuleManager::CheckNoInit( const std::wstring_view module_name )
    {
        if ( module_name.find( L"GraphicInterface" ) != std::wstring::npos )
        {
            assert( g_graphic_api );
            return true;
        }

        if ( module_name == L"Memory" )
        {
            return true;
        }

        if ( module_name == L"CoreModule" )
        {
            return true;
        }

#if PLATFORM == Windows
        if ( module_name == L"WinAPIWrapper" )
        {
            return true;
        }
#endif

        return false;
    }
#endif

#if IS_DLL
    bool ModuleManager::CheckNoInit( const ModuleInfo* module_info )
    {
        // Ignore load/unload self
        if ( module_info->m_handle_ == g_module_api->m_handle_ )
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

        // Memory and type management module should be loaded before the module manager.
        if ( module_info->m_handle_ == g_core_mem->m_handle_ )
        {
            return true;
        }

        // Core libraries are loaded before the module manager.
        if ( module_info->m_handle_ == g_core_api->m_handle_ )
        {
            return true;
        }
        return false;
    }
#endif

	void ModuleManager::Initialize()
	{
		m_module_paths_.emplace(L"Default", "./");
    }

    void ModuleManager::Shutdown()
    {
        // Remove the dummy core module info.
        for ( auto it = m_module_loaded_.begin(); it != m_module_loaded_.end();)
        {
            if (
#if IS_DLL
                CheckNoInit( it->second.get() )
#else
                CheckNoInit(it->first )
#endif
            )
            {
                it = m_module_loaded_.erase( it );
            }
            else
            {
                ++it;
            }
        }

        // Unload in reverse load order (dependents before dependencies).
        for ( auto it = m_module_load_order_.rbegin(); it != m_module_load_order_.rend(); ++it )
        {
            ShutdownModule( *it );
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

                    m_module_load_order_.emplace_back( name );
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
                if ( !CheckNoInit( name.data() ) )
                {
                    if ( const ModuleInitializationFunction& func = m_module_initializer_.at( name.data() ) )
                    {
                        module_info->m_module_ = std::unique_ptr<IModule>( func() );
                        // Assuming that the dependent libraries are loaded.
                        module_info->m_b_lazy = false;

                        if ( module_info->m_module_ )
                        {
                            for ( const std::string_view dependency : module_info->m_module_->GetDependencies() )
                            {
                                std::wstring conversion( dependency.begin(), dependency.end() );

                                if ( CheckNoInit( conversion ) )
                                {
                                    continue;
                                }

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
                            m_module_load_order_.emplace_back( name.begin(), name.end() );
                        }
                    }
                    else
                    {
                        return nullptr;
                    }
                }

                if ( !module_info->m_b_lazy )
                {
                    std::string conversion( name.begin(), name.end() );
                    CONSOLE_OUT( "ModuleManager", "Module {} loaded", conversion.c_str() )

                    TryResolveLazyness( name );
                    return module_info->m_module_.get();
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

                        module_info->m_module_->SetModuleName( name );
                        module_info->m_module_->Initialize();
                        m_module_load_order_.emplace_back( name.begin(), name.end() );
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

    void ModuleManager::Destroy()
    {
        for ( auto it = m_module_load_order_.rbegin(); m_module_load_order_.rend() != it; ++it )
        {
            if ( !m_module_loaded_.contains(*it) )
            {
                continue;
            }

            if ( m_module_loaded_.at( *it )->m_module_ )
            {
                throw std::runtime_error( "Module does not shutdown" );
            }

            if ( ModuleInfoPtr ptr = std::move( m_module_loaded_.at(*it) ) )
            {
#if IS_DLL
#if PLATFORM == Windows
                if ( HMODULE module = static_cast<HMODULE>( ptr->m_handle_ ) )
                {
                    FreeLibrary( module );
                }
#endif
#endif
                ptr.reset();
                m_module_loaded_.erase( *it );
            }
        }

        m_module_load_order_.clear();
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

	void ModuleManager::RegisterOnModuleShutdown( OnModuleShutdownCallback cb )
	{
		if ( cb )
		{
			std::lock_guard l( m_read_mutex_ );
			m_on_module_shutdown_.push_back( std::move( cb ) );
		}
	}

	void ModuleManager::ShutdownModule(const std::wstring_view name)
	{
		{
			std::lock_guard l(m_write_mutex_);
			if (!m_module_loaded_.contains(name.data()))
			{
				return;
			}
		}
		
		std::lock_guard l(m_read_mutex_);
        const ModuleInfoPtr& module_info = m_module_loaded_.at( name.data() );

		if (module_info)
		{
            if (
#if IS_DLL
                GetModuleHandleW( module_info->m_path_.c_str() ) && 
#endif
                module_info->m_module_ )
            {
				for ( const OnModuleShutdownCallback& callback : m_on_module_shutdown_ )
				{
					callback( name );
				}
                module_info->m_module_->Shutdown();
                module_info->m_module_.reset();
            }
		}
    }

    bool ModuleManager::CreateModuleOnly( const std::wstring_view name )
    {
#if !IS_DLL
        ModuleInitializationFunction func;
        {
            std::lock_guard l( m_read_mutex_ );
            if ( CheckNoInit( name ) || !m_module_initializer_.contains( name.data() ) )
            {
                return false;
            }
            func = m_module_initializer_.at( name.data() );
        }
        AddModule( name );
        {
            std::lock_guard l( m_read_mutex_ );
            ModuleInfo* module_info = FindModule( name );
            if ( !module_info )
            {
                return false;
            }
            module_info->m_module_ = std::unique_ptr<IModule>( func() );
            module_info->m_b_lazy = false;
            return module_info->m_module_ != nullptr;
        }
#else
        AddModule( name );
        std::lock_guard l( m_read_mutex_ );
        ModuleInfo* module_info = FindModule( name );
        if ( !module_info || CheckNoInit( module_info ) )
        {
            return false;
        }
        if ( const HMODULE hModule = GetModuleHandleW( module_info->m_path_.c_str() ) )
        {
            module_info->m_handle_    = hModule;
            module_info->m_b_dynamic_ = false;
        }
        else
        {
            module_info->m_handle_ = LoadLibraryW( module_info->m_path_.c_str() );
            module_info->m_b_dynamic_ = true;
        }
        if ( !module_info->m_handle_ )
        {
            module_info->m_last_error_ = GetLastError();
            return false;
        }
        const ModuleInitializationFunctionCStyle& init_func =
            ( ModuleInitializationFunctionCStyle )GetProcAddress(
                static_cast<HMODULE>( module_info->m_handle_ ), "InitializeModule" );
        if ( !init_func )
        {
            return false;
        }
        module_info->m_module_ = std::unique_ptr<IModule>( init_func() );
        module_info->m_b_lazy = false;
        return module_info->m_module_ != nullptr;
#endif
    }

    void ModuleManager::InitializeModuleInOrder( const std::wstring_view name )
    {
        std::lock_guard l( m_read_mutex_ );
        ModuleInfo* module_info = FindModule( name );
        if ( !module_info || !module_info->m_module_ )
        {
            return;
        }
#if IS_DLL
        module_info->m_module_->SetModuleName( name );
#endif
        module_info->m_module_->Initialize();
        m_module_load_order_.emplace_back( name.begin(), name.end() );
        std::string conversion( name.begin(), name.end() );
        CONSOLE_OUT( "ModuleManager", "Module {} loaded", conversion.c_str() );
    }

    std::vector<std::wstring> ModuleManager::SortByPhaseAndDependency( const std::vector<std::wstring>& names )
    {
        if ( names.empty() )
        {
            return {};
        }
        const std::unordered_set<std::wstring> name_set( names.begin(), names.end() );

        // Build reverse adjacency: for each B in deps(A), rev_adj[B].push_back(A). Edge B->A means B before A.
        std::unordered_map<std::wstring, std::vector<std::wstring>> rev_adj;
        for ( const std::wstring& a : names )
        {
            ModuleInfo* info = FindModule( a );
            if ( !info || !info->m_module_ )
            {
                continue;
            }
            IModule* m = info->m_module_.get();
            for ( const std::string_view dep : m->GetDependencies() )
            {
                std::wstring wdep( dep.begin(), dep.end() );
                if ( name_set.contains( wdep ) && wdep != a )
                {
                    rev_adj[wdep].push_back( a );
                }
            }
        }

        // Sort by phase first: group by phase, then topo within each phase.
        std::unordered_map<std::wstring, ELoadPhase> phase_of;
        for ( const std::wstring& n : names )
        {
            ModuleInfo* info = FindModule( n );
            phase_of[n] = ( info && info->m_module_ ) ? info->m_module_->GetLoadPhase() : ELoadPhase::Internal;
        }
        std::vector<std::wstring> sorted;
        sorted.reserve( names.size() );
        for ( int p = 0; p <= static_cast<int>( ELoadPhase::Application ); ++p )
        {
            ELoadPhase phase = static_cast<ELoadPhase>( p );
            std::vector<std::wstring> in_phase;
            for ( const std::wstring& n : names )
            {
                if ( phase_of[n] == phase )
                {
                    in_phase.push_back( n );
                }
            }
            const size_t phase_start = sorted.size();
            // Topological sort within phase (Kahn). Only count in-phase dependencies.
            std::unordered_map<std::wstring, int> in_deg_phase;
            for ( const std::wstring& n : in_phase )
            {
                in_deg_phase[n] = 0;
            }
            for ( const std::wstring& n : in_phase )
            {
                ModuleInfo* info = FindModule( n );
                if ( !info || !info->m_module_ )
                {
                    continue;
                }
                IModule* m = info->m_module_.get();
                for ( const std::string_view dep : m->GetDependencies() )
                {
                    std::wstring wdep( dep.begin(), dep.end() );
                    if ( name_set.contains( wdep ) && phase_of[wdep] == phase && wdep != n )
                    {
                        in_deg_phase[n]++;
                    }
                }
            }
            std::queue<std::wstring> q;
            for ( const std::wstring& n : in_phase )
            {
                if ( in_deg_phase[n] == 0 )
                {
                    q.push( n );
                }
            }
            while ( !q.empty() )
            {
                std::wstring n = std::move( q.front() );
                q.pop();
                sorted.push_back( n );
                auto it = rev_adj.find( n );
                if ( it != rev_adj.end() )
                {
                    for ( const std::wstring& m : it->second )
                    {
                        if ( phase_of[m] == phase && --in_deg_phase[m] == 0 )
                        {
                            q.push( m );
                        }
                    }
                }
            }
            // If any in_phase not in sorted (cycle), append rest arbitrarily.
            for ( const std::wstring& n : in_phase )
            {
                if ( std::find( sorted.begin() + static_cast<ptrdiff_t>( phase_start ), sorted.end(), n ) == sorted.end() )
                {
                    sorted.push_back( n );
                }
            }
        }
        return sorted;
    }

	void ModuleManager::LoadModuleAll()
	{
#if IS_DLL
        auto is_core_module = []( const std::wstring& name ) -> bool
        {
            for ( std::wstring_view core : kCoreModuleLoadOrder )
            {
                if ( name == core )
                {
                    return true;
                }
            }
            return name.ends_with( L"GraphicInterface" );
        };

        std::vector<std::wstring> names;
        for ( const auto& directory : m_module_paths_ )
        {
            for ( const auto& entry : std::filesystem::directory_iterator( directory.second ) )
            {
                if ( const std::wstring file_name = entry.path().stem().generic_wstring();
                     entry.is_regular_file() && entry.path().extension() == L".dll" && !is_core_module( file_name ) )
                {
                    names.push_back( file_name );
                }
            }
        }
        for ( const std::wstring& name : names )
        {
            AddModule( name );
        }
        std::vector<std::wstring> created;
        for ( const std::wstring& name : names )
        {
            if ( CreateModuleOnly( name ) )
            {
                created.push_back( name );
            }
        }
        std::vector<std::wstring> ordered = SortByPhaseAndDependency( created );
        for ( const std::wstring& name : ordered )
        {
            InitializeModuleInOrder( name );
        }
#else
        std::vector<std::wstring> names;
        for ( const auto& [name, func] : m_module_initializer_ )
        {
            names.push_back( name );
        }
        std::vector<std::wstring> created;
        for ( const std::wstring& name : names )
        {
            if ( CreateModuleOnly( name ) )
            {
                created.push_back( name );
            }
        }
        std::vector<std::wstring> ordered = SortByPhaseAndDependency( created );
        for ( const std::wstring& name : ordered )
        {
            InitializeModuleInOrder( name );
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
