#include "pch.hpp"
#include "egEntity.hpp"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>

SERIALIZER_ACCESS_IMPL(Engine::Abstract::Entity,
	_ARTAG(m_name_))