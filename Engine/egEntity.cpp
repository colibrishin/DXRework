#include "pch.hpp"
#include "egEntity.hpp"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>

#include "imgui_internal.h"
#include "imgui_stdlib.h"

SERIALIZER_ACCESS_IMPL(Engine::Abstract::Entity,
                       _ARTAG(m_name_))

void Engine::Abstract::Entity::Initialize()
{
	m_b_initialized_ = true;
}

void Engine::Abstract::Entity::OnDeserialized()
{
	if (m_b_initialized_)
	{
		throw std::runtime_error("Entity already initialized");
	}

	m_b_initialized_ = true;
}

void Engine::Abstract::Entity::OnImGui()
{
	ImGui::Indent(2);
	ImGui::Text("Entity ID: %d", GetID());
	ImGui::InputText("Entity Name", &m_name_);
	ImGui::Unindent(2);
}
