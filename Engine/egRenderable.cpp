#include "pch.h"
#include "egRenderable.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>

SERIALIZER_ACCESS_IMPL(Engine::Abstract::Renderable, _ARTAG(_BSTSUPER(Entity)))
