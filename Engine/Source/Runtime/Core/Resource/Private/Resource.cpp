#include "../Public/Resource.h"

SERIALIZE_IMPL
(
 Engine::Abstracts::Resource,
 _ARTAG(_BSTSUPER(Engine::Abstracts::Entity))
 _ARTAG(m_bLoaded_)
 _ARTAG(m_path_)
 _ARTAG(m_type_)
)

namespace Engine::Abstracts
{
	Abstracts::Resource::~Resource() {}

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

	Resource::Resource(boost::filesystem::path path, eResourceType type)
		: m_bLoaded_(false),
		  m_type_(type),
		  m_path_(std::move(path))
	{
	}

	Resource::Resource()
		: m_bLoaded_(false),
		  m_type_(static_cast<eResourceType>(0)) {}

	void Resource::OnDeserialized()
	{
		Entity::OnDeserialized();
		m_bLoaded_ = false;
	}

	bool Resource::IsLoaded() const
	{
		return m_bLoaded_;
	}

	const boost::filesystem::path& Resource::GetPath() const
	{
		return m_path_;
	}

	void Resource::SetPath(const boost::filesystem::path& path)
	{
		m_path_     = path;
	}

	eResourceType Resource::GetResourceType() const
	{
		return m_type_;
	}
} // namespace Engine::Abstract
