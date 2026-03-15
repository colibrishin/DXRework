#include "ComputeShader.h"

#if WITH_DEBUG
#include "Debugger.h"
#endif

#include "IGraphicAPI.h"
#include "ResourceManager.h"

namespace Engine::Resources
{
	void ComputeShader::Dispatch(const IGraphicContext* context, const UINT group_count[3], Graphics::SBs::LocalParamSB& param, const float dt)
	{
		IGraphicAPI& gi = g_graphic_accessor.GetInterface();
		preDispatch( context, param, dt );
		gi.Dispatch(context, this, param, group_count);
		postDispatch( context, param, dt );
	}

	IComputeShader& ComputeShader::GetComputePrimitiveShader() const
	{
		return *m_primitive_shader_;
	}

#if WITH_EDITOR
    void ComputeShader::OnUIUpdate( UIContext *const parent, const float dt )
    {
        if ( parent )
        {
            Resource::OnUIUpdate( parent, dt );
            IUIAPI &ui = g_ui_accessor.GetInterface();
            *parent |= ui.NewLabelAndUInt( this,
                                           "ThreadGroupX",
                                           { "Thread Group X", m_thread_[ 0 ], 0, 0, 255, true } );
            *parent |= ui.NewLabelAndUInt( this,
                                           "ThreadGroupY",
                                           { "Thread Group Y", m_thread_[ 1 ], 0, 0, 255, true } );
            *parent |= ui.NewLabelAndUInt( this,
                                           "ThreadGroupZ",
                                           { "Thread Group Z", m_thread_[ 2 ], 0, 0, 255, true } );
        }
	}
#endif

	ComputeShader::ComputeShader( const std::filesystem::path& path )
		: Resource(path)
    {}

    void ComputeShader::SetThread( const std::array<uint32_t, 3> &thread )
    { 
		m_thread_ = thread;
	}

	ComputeShader::ComputeShader(const ComputeShader& other)
		: Resource(other)
	{
		m_thread_ = other.m_thread_;
	}

	ComputeShader& ComputeShader::operator=(const ComputeShader& other)
	{
		m_thread_ = other.m_thread_;
		return *this;
	}

	std::array<UINT, 3> ComputeShader::GetThread() const
	{
		std::array<UINT, 3> thread;
		thread[0] = m_thread_[0];
		thread[1] = m_thread_[1];
		thread[2] = m_thread_[2];
		return thread;
	}

	void ComputeShader::PostUpdate(const float dt) {}

	void ComputeShader::PreUpdate(const float dt) {}

	void ComputeShader::FixedUpdate(const float dt) {}

	void ComputeShader::Update(const float dt) {}

	void ComputeShader::Initialize()
	{
		Resource::Initialize();
	}

	void ComputeShader::Load_INTERNAL()
	{
		m_primitive_shader_ = Unique<IComputeShader>(g_graphic_accessor.GetInterface().GetNewComputePrimitiveShader());
		m_primitive_shader_->Generate(this, g_graphic_accessor.GetInterface().GetNativePipeline());
		
		loadDerived();
	}

	void ComputeShader::Unload_INTERNAL()
	{
		m_primitive_shader_.reset();

		unloadDerived();
	}

	ComputeShader::ComputeShader()
		: Resource (""),
		  m_thread_{1,} {}

	void ComputeShader::OnSerialized()
	{
		if (exists(GetPath()))
		{
			const std::filesystem::path folder   = GetPrettyTypeName();
			const std::filesystem::path filename = GetPath().filename();
			const std::filesystem::path p        = folder / filename;

			if (!exists(folder))
			{
				create_directory(folder);
			}

			if (GetPath() == p)
			{
				return;
			}

			if (exists(p))
			{
				std::filesystem::remove(p);
			}

			copy_file(GetPath(), p, std::filesystem::copy_options::overwrite_existing);

			SetPath(p);
		}
	}
}
