#include "Resource.h"

namespace Engine::Abstracts
{
	Resource::~Resource() {}

	Resource::Resource(const Resource& other)
		: Entity( other ), m_bLoaded_( false )
	{
		m_path_ = other.m_path_;
	}

	Resource& Resource::operator=(const Resource& other)
	{
		m_path_ = other.m_path_;
		return *this;
	}

	void Resource::Load()
	{
		if (!m_bLoaded_)
		{
			Load_INTERNAL();
			m_bLoaded_ = true;
		}
	}

	void Resource::Unload()
	{
		if (m_bLoaded_)
		{
			Unload_INTERNAL();
			m_bLoaded_ = false;
		}
	}

	Resource::Resource(std::filesystem::path path)
		: m_bLoaded_(false),
		  m_path_(std::move(path))
	{
	}

	Resource::Resource()
		: m_bLoaded_(false) {}

#if WITH_EDITOR
    void Resource::OnUIUpdate( UIContext *const parent, const float dt )
    {
        if ( parent )
        {
            Entity::OnUIUpdate( parent, dt );
            IUIAPI &ui = g_ui_accessor.GetInterface();
            *parent |= ui.NewLabelAndPath( this, "RawPath", { "Raw Path", m_path_ } );
            ( *parent |= ui.NewButton( this, "CloneButton", { "Clone" } ) )
                    .SetFunction(
                            [ this ]()
                            {
                                const auto &cloned = Clone();
                                Managers::ResourceManager::GetInstance().AddResource( cloned, cloned->GetTypeHash() );
                            } );
        }
	}
#endif

	void Resource::OnDeserialized()
	{
		Entity::OnDeserialized();
		m_bLoaded_ = false;
	}

	bool Resource::IsLoaded() const
	{
		return m_bLoaded_;
	}

	const std::filesystem::path& Resource::GetPath() const
	{
		return m_path_;
	}

	Strong<Resource> Resource::Clone() const
	{
		Strong<Resource> cloned = cloneImpl();

		size_t idx = 0;
		std::string new_name;
		while ( true )
		{
			new_name = std::format( "{}_{}", cloned->GetName(), idx );
			if ( const Strong<Resource>& res = Managers::ResourceManager::GetInstance().GetResource( new_name, cloned->GetTypeHash() ).lock();
				 res == nullptr )
			{
				break;
			}
		}
		cloned->SetName( new_name );
		Serializer::Serialize( cloned->GetName(), cloned );
		return cloned;
	}

	void Resource::SetPath(const std::filesystem::path& path)
	{
		m_path_     = path;
	}
} // namespace Engine::Abstract
