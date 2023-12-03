#include "pch.hpp"
#include "egResource.hpp"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>

SERIALIZER_ACCESS_IMPL(Engine::Abstract::Resource)
SERIALIZER_ACCESS_IMPL2(Engine::Abstract::Resource,
	_ARTAG(_BASEOBJECT(Renderable))
	_ARTAG(m_bLoaded_)
	_ARTAG(m_path_wstr_)
	_ARTAG(m_priority_))

namespace Engine::Abstract
{
	Resource::~Resource()
	{
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

	Resource::Resource(std::filesystem::path path, eResourcePriority priority): m_bLoaded_(false), m_path_(std::move(path)), m_priority_(priority)
	{
		m_path_wstr_ = m_path_.wstring();
	}

	void Resource::AfterDeserialized()
	{
		m_bLoaded_ = false;
		m_path_ = m_path_wstr_;
		Load();
	}
}
