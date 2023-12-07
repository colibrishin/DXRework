#include "pch.hpp"
#include "egResource.hpp"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>

#include <imgui_stdlib.h>

SERIALIZER_ACCESS_IMPL(Engine::Abstract::Resource,
	_ARTAG(_BSTSUPER(Renderable))
	_ARTAG(m_bLoaded_)
	_ARTAG(m_path_str_)
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

	void Resource::OnImGui()
	{
		ImGui::BulletText(ToTypeName().c_str());
		Renderable::OnImGui();
		ImGui::Indent(2);
		ImGui::Checkbox("Loaded", &m_bLoaded_);
		ImGui::Text("Path : %s", m_path_str_);
		ImGui::Unindent(2);
	}

	Resource::Resource(std::filesystem::path path, eResourcePriority priority): m_bLoaded_(false), m_path_(std::move(path)), m_priority_(priority)
	{
		m_path_str_ = m_path_.string();
	}

	void Resource::OnDeserialized()
	{
		m_bLoaded_ = false;
		m_path_ = m_path_str_;
	}
}
