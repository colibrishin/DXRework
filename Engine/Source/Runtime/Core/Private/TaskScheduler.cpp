#include "TaskScheduler.h"
#include "TaskScheduler.generated.h"
#include "EngineEntryPoint.h"

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
                                                        return elem.target<void(*)(const std::vector<std::any> &, float)>() ==
                                                            func.target<void(*)(const std::vector<std::any> &, float)>();
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
                      return elem.target<void( * )( const std::vector<std::any> &, float )>() ==
                          func.target<void( * )(const std::vector<std::any> &, float)>();
                  } );
    }
    void TaskScheduler::PreDeconstruction()
    {
        PreUpdate(EngineEntryPoint::GetInstance().GetDeltaTime());
        m_injected_funcs_.clear();
        m_tasks_.clear();
    }
}
