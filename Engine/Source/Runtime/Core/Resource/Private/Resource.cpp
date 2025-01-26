#include "../Public/Resource.h"
#include "Resource.generated.h"

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
	void Resource::OnUIUpdate(UIContext* const parent, const float dt)
	{
		if (parent)
		{
			Entity::OnUIUpdate(parent, dt);
			UIInterface& ui = UIInterfaceAccessor::GetInterface();
			*parent |= ui.NewLabelAndPath({ "Raw Path", m_path_ });
			( *parent |= ui.NewButton( {"Clone"} ) ).SetFunction( [this]()
			{
				const auto& res = Clone();

				if (const Strong<Resource>& locked = res.lock())
				{
					Managers::ResourceManager::GetInstance().AddResource( locked, locked->GetTypeHash() );
				}
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

	Weak<Resource> Resource::Clone() const
	{
		Strong<Resource> cloned = cloneImpl();

		size_t idx = 0;
		std::string new_name;
		while ( true )
		{
			new_name = std::format( "{} {}", cloned->GetName(), idx );
			if ( const Strong<Resource>& res = Managers::ResourceManager::GetInstance().GetResource( new_name, cloned->GetTypeHash() ).lock();
				 res == nullptr )
			{
				break;
			}
		}
		cloned->SetName( new_name );
		return cloned;
	}

	void Resource::SetPath(const std::filesystem::path& path)
	{
		m_path_     = path;
	}
} // namespace Engine::Abstract
