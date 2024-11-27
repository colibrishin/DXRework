#include "pch.h"
#include "egRenderable.h"

#include <boost/serialization/export.hpp>

SERIALIZE_IMPL(Engine::Abstract::Renderable, _ARTAG(_BSTSUPER(Entity)))
