#include "pch.hpp"
#include "egRenderable.hpp"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>

SERIALIZER_ACCESS_IMPL3(Engine::Abstract::Renderable,
	_ARTAG(_BASEOBJECT(Entity)))