#include "Renderer.h"

#include <algorithm>

#include "ConcurrentTypeLibrary.h"
#include "ModuleManager.h"
#include "RenderPipeline.h"
#include "Scene.h"
#include "SceneManager.h"

namespace Engine::Managers
{
	Renderer::~Renderer() {}

	void Renderer::PreUpdate(const float dt)
	{
		for (int i = 0; i < SHADER_DOMAIN_MAX; ++i)
		{
            for ( size_t& count : m_render_pass_tasks_usage_[ i ] | std::views::values )
            {
                count = 0;
            }
		}

		for (const auto& ptr : m_render_instance_tasks_ | std::views::values) 
		{
		    ptr->Cleanup(m_render_candidates_, SHADER_DOMAIN_MAX);
		}

		m_b_ready_ = false;
	}

	void Renderer::Update(const float dt) {}

	void Renderer::FixedUpdate(const float dt) {}

	void Renderer::PreRender(const float dt)
	{
		if (const auto& scene = SceneManager::GetInstance().GetActiveScene().lock()) 
		{
			for (const auto& ptr : m_render_instance_tasks_ | std::views::values)
			{
				ptr->Run(scene.get(), m_render_candidates_, SHADER_DOMAIN_MAX);
			}
		}
		else
		{
			// Nothing to draw: tasks that keep instances across frames drop them.
			for (const auto& task : m_render_instance_tasks_ | std::views::values)
			{
				task->Release(m_render_candidates_, SHADER_DOMAIN_MAX);
			}
		}
	    
		m_b_ready_ = true;
	}

	void Renderer::Render(const float dt)
	{
		for (size_t i = 0; i < SHADER_DOMAIN_MAX; ++i)
		{
			// Sorted every frame (a domain has a few passes): a pass may come or go with its module.
			std::vector<std::pair<const std::wstring*, IRenderPassTaskFactory*>> passes;
			passes.reserve( m_render_pass_tasks_factories_[ i ].size() );
			for ( const auto& [ name, factory ] : m_render_pass_tasks_factories_[ i ] )
			{
				passes.emplace_back( &name, factory );
			}
			std::ranges::sort( passes,
			                   []( const auto& a, const auto& b )
			                   {
				                   const int oa = a.second->GetOrder();
				                   const int ob = b.second->GetOrder();
				                   return oa != ob ? oa < ob : *a.first < *b.first;
			                   } );

			for ( IRenderPassTaskFactory* factory : passes | std::views::values )
			{
                RenderPassAssisted( factory,
                                    dt,
                                    false,
                                    static_cast<eShaderDomain>( i ),
                                    {},
                                    m_additional_sbs_,
                                    {},
                                    {},
                                    {} );
			}
            
			onRenderDone.Broadcast(static_cast<eShaderDomain>(i));
		}
	}

	void Renderer::PostRender(const float dt)
	{
	}

	void Renderer::PostUpdate(const float dt) {}

	void Renderer::Initialize()
	{
#if IS_DLL
		ModuleManager::GetInstance().RegisterOnModuleShutdown(
			[]( std::wstring_view name )
			{
				// Modules after RenderPipeline still shut down; GetInstance() would create a new Renderer that is never freed.
				if ( Renderer::IsInitialized() )
				{
					Renderer::GetInstance().UnregisterModule( name );
				}
			} );
#endif
	}

	void Renderer::RegisterRenderInstance(const std::wstring_view name, RenderInstanceTask* task)
	{
		if (task != nullptr) 
		{
			m_render_instance_tasks_.emplace(name, std::unique_ptr<RenderInstanceTask>(task));
		}
	}

	void Renderer::RegisterRenderPass( const std::wstring_view name, IRenderPassTaskFactory* task, const std::wstring_view module_name )
	{
		if ( task != nullptr )
		{
			m_unique_render_pass_task_factories_.emplace( name, std::unique_ptr<IRenderPassTaskFactory>( task ) );
#if IS_DLL
			if ( !module_name.empty() )
			{
				m_render_pass_names_by_module_[ std::wstring( module_name ) ].insert( std::wstring( name ) );
			}
#endif
		}
	}

	void Renderer::RenderPassWith( const std::wstring_view name, const eShaderDomain domain, const std::wstring_view module_name )
	{
		if ( domain < SHADER_DOMAIN_BEGIN || domain >= SHADER_DOMAIN_MAX )
		{
			return;
		}

		if ( m_unique_render_pass_task_factories_.contains( name.data() ) )
		{
			m_render_pass_tasks_factories_[ domain ].emplace(
				name.data(),
				m_unique_render_pass_task_factories_.at( name.data() ).get() );
#if IS_DLL
			if ( !module_name.empty() )
			{
				m_render_pass_names_by_module_[ std::wstring( module_name ) ].insert( std::wstring( name ) );
			}
#endif
			onRenderTaskDirty.Broadcast();
		}
	}

	void Renderer::UnregisterRenderInstance(const std::wstring_view name)
	{
		if (const auto it = m_render_instance_tasks_.find(name.data()); it != m_render_instance_tasks_.end())
		{
			// The task's instances must leave the render map before the task (and its buffers) goes away.
			it->second->Release(m_render_candidates_, SHADER_DOMAIN_MAX);
			m_render_instance_tasks_.erase(it);
		}
	}

	void Renderer::UnregisterRenderPass( const std::wstring_view name )
	{
		if ( m_unique_render_pass_task_factories_.contains( name.data() ) )
		{
			IRenderPassTaskFactory* factory = m_unique_render_pass_task_factories_.at( name.data() ).get();
			HashType                task_type = factory->GetTaskType();

			for ( size_t i = 0; i < SHADER_DOMAIN_MAX; ++i )
			{
				m_render_pass_tasks_factories_[ i ].erase( name.data() );

				for ( RenderPassTask* task : m_render_pass_tasks_[ i ][ task_type ] )
				{
					factory->Release( task );
				}

				// Drop the released tasks: RenderPass() indexes them by the per-frame use count.
				m_render_pass_tasks_[ i ].erase( task_type );
				m_render_pass_tasks_usage_[ i ].erase( task_type );
			}

			m_unique_render_pass_task_factories_.erase( name.data() );
#if IS_DLL
			const std::wstring name_str( name );
			for ( auto& [ mod, names ] : m_render_pass_names_by_module_ )
			{
				names.erase( name_str );
			}
#endif
			onRenderTaskDirty.Broadcast();
		}
	}

	void Renderer::UnregisterModule( std::wstring_view module_name )
	{
#if IS_DLL
		const std::wstring key( module_name );
		auto it = m_render_pass_names_by_module_.find( key );
		if ( it == m_render_pass_names_by_module_.end() )
		{
			return;
		}
		// UnregisterRenderPass erases the name from this same container: iterate a copy.
		const auto names = it->second;
		for ( const std::wstring& name : names )
		{
			UnregisterRenderPass( name );
		}
		m_render_pass_names_by_module_.erase( key );
#endif
	}

	void Renderer::RenderPassWithout( const std::wstring_view name, const eShaderDomain domain )
    {
        if ( domain < SHADER_DOMAIN_BEGIN || domain >= SHADER_DOMAIN_MAX )
        {
            return;
        }

		if ( m_unique_render_pass_task_factories_.contains( name.data() ) && 
			 m_render_pass_tasks_factories_[ domain ].contains( name.data() ) )
		{
            IRenderPassTaskFactory* factory = m_unique_render_pass_task_factories_.at( name.data() ).get();
            HashType                task_type = factory->GetTaskType();
            m_render_pass_tasks_factories_[ domain ].erase( name.data() );

			for (RenderPassTask* task : m_render_pass_tasks_[domain][task_type])
			{
                factory->Release( task );
			}

            m_render_pass_tasks_[ domain ].erase( task_type );
            m_render_pass_tasks_usage_[ domain ].erase( task_type );
		}
	}

	void Renderer::RegisterStructuredBuffer(const StructuredBufferDecorator* sb)
	{
		if (const auto& it = std::ranges::find(m_additional_sbs_, sb);
			it == m_additional_sbs_.end())
		{
			m_additional_sbs_.push_back(sb);
		}
	}
	
	void Renderer::UnregisterStructuredBuffer(const StructuredBufferDecorator* sb)
	{
		std::erase_if(m_additional_sbs_, [&sb](const StructuredBufferDecorator* elem)
		{
			return elem == sb;
		});
	}

	void Renderer::RegisterContextPreRenderSetup(
		const std::string_view name, const ContextSetupFunction& prerender_func)
	{
		if (!m_prerender_funcs_.contains(name))
		{
			m_prerender_funcs_.emplace(name, prerender_func);	
		}
	}

	void Renderer::UnregisterContextPreRenderSetup(const std::string_view name)
	{
		if (m_prerender_funcs_.contains(name))
		{
			m_prerender_funcs_.erase(name);
		}
	}

	void Renderer::RegisterContextPostRenderSetup(
		const std::string_view name, const ContextSetupFunction& postrender_func)
	{
		if (!m_postrender_funcs_.contains(name))
		{
			m_postrender_funcs_.emplace(name, postrender_func);
		}
	}

	void Renderer::UnregisterContextPostRenderSetup(const std::string_view name)
	{
		if (m_postrender_funcs_.contains(name))
		{
			m_postrender_funcs_.erase(name);
		}
    }

#if IS_DLL
    Renderer::BorrowedRenderPassFactories&
    Renderer::GetRenderTasks( const RenderTaskTraits& traits, const std::vector<HashType>& types, bool inclusion )
    {
        BorrowedRenderPassFactories& borrowed_factories_ = GetFactories( traits );
        std::once_flag               delegate_flag;

        // By reference: the binder stores decayed copies of its arguments, so the list it re-resolved was its own.
        auto tasks  = std::ref( borrowed_factories_ );
        auto binder = managed_bind( &Renderer::ResolveDirty, GetWeakPtr<Renderer>(), types, tasks, inclusion );
        onRenderTaskDirty.Listen( binder );
        // Passes registered before this first call do not signal again.
        ResolveDirty( types, borrowed_factories_, inclusion );

		return borrowed_factories_;
    }

    Renderer::BorrowedRenderPassFactories& Renderer::GetFactories( const RenderTaskTraits& traits )
    {
        if ( !m_render_task_resolvers_.contains( traits ) )
        {
            m_render_task_resolvers_.emplace( traits, BorrowedRenderPassFactories{} );
        }

        return m_render_task_resolvers_[ traits ];
    }
    void
    Renderer::ResolveDirty( const std::vector<HashType>& types, BorrowedRenderPassFactories& tasks, bool inclusion )
    { 
		tasks.clear();
        tasks.reserve( m_unique_render_pass_task_factories_.size() );

        for ( const Unique<IRenderPassTaskFactory>& factory :
              m_unique_render_pass_task_factories_ | std::views::values )
        {
            if ( std::any_of( types.begin(), types.end(), [ &factory ]( const HashType& h ) { return h == factory->GetTaskType(); } ) )
            {
                if ( inclusion && factory != nullptr )
                {
                    tasks.emplace_back( factory.get() );
				}
                else
                {
                    continue;
				}
            }
            else
            {
                if ( !inclusion && factory != nullptr )
                {
                    tasks.emplace_back( factory.get() );
				}
			}
        }
	}
#endif

	bool Renderer::Ready() const
	{
		return m_b_ready_;
	}
}
