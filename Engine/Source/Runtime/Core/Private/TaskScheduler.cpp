#include "TaskScheduler.h"
#include "EngineEntryPoint.h"

namespace
{
    using TaskFuncPtr = void ( * )( const std::vector<std::any>&, float );

    // Same registration only when both wrap the same plain function. Other callables (lambdas, binds) have no
    // comparable identity and never match.
    bool IsSameFunction( const Engine::TaskSchedulerFunc& lhs, const Engine::TaskSchedulerFunc& rhs )
    {
        const TaskFuncPtr* lhs_ptr = lhs.target<TaskFuncPtr>();
        const TaskFuncPtr* rhs_ptr = rhs.target<TaskFuncPtr>();
        return lhs_ptr && rhs_ptr && *lhs_ptr == *rhs_ptr;
    }
}

namespace Engine::Managers
{
	void TaskScheduler::Initialize() {}

	void TaskScheduler::PreUpdate(const float dt)
	{
		for (int i = 0; i < TASK_MAX; ++i)
		{
			while (!m_tasks_[static_cast<eTaskType>(i)].empty())
			{
                auto [type, given_func, params] = m_tasks_[ static_cast<eTaskType>( i ) ].front();
                m_tasks_[ static_cast<eTaskType>( i ) ].pop();
			    if ( given_func )
			    {
			        given_func( params, dt );   
			    }
                for ( const TaskSchedulerFunc &inject_func : m_injected_funcs_[ ( eTaskType )i ] )
                {
                    inject_func( params, dt );
                }
			}
		}
	}

	void TaskScheduler::Update(const float dt) {}

	void TaskScheduler::PreRender(const float dt) {}

	void TaskScheduler::Render(const float dt) {}

	void TaskScheduler::PostRender(const float dt) {}

	void TaskScheduler::PostUpdate(const float dt) {}

	void TaskScheduler::FixedUpdate(const float dt) {}

    void TaskScheduler::Inject( const eTaskType type, const TaskSchedulerFunc &func )
	{
        if ( const auto &it = std::ranges::find_if(
                                                    m_injected_funcs_[ type ],
                                                    [&func](const TaskSchedulerFunc& elem)
                                                    {
                                                        return IsSameFunction( elem, func );
                                                    });
            it == m_injected_funcs_[ type ].end() )
        {
            m_injected_funcs_[ type ].emplace_back( func );
        }
	}

    void TaskScheduler::Extract( const eTaskType type, const TaskSchedulerFunc &func )
	{
        std::erase_if
                ( m_injected_funcs_[ type ],
                  [&func]( const TaskSchedulerFunc &elem )
                  {
                      return IsSameFunction( elem, func );
                  } );
    }
    void TaskScheduler::PreDeconstruction()
    {
        m_injected_funcs_.clear();
        m_tasks_.clear();
    }
}
