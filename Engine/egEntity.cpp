#include "pch.hpp"
#include "egEntity.hpp"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>

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
