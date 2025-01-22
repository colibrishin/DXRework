#include "../Public/Resource.h"

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

	Resource::Resource(std::filesystem::path path)
		: m_bLoaded_(false),
		  m_path_(std::move(path))
	{
	}

	Resource::Resource()
		: m_bLoaded_(false) {}

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

	void Resource::SetPath(const std::filesystem::path& path)
	{
		m_path_     = path;
	}
} // namespace Engine::Abstract
