#include "GraphicInterface.h"

Engine::Unique<Engine::PolymorphicGraphicInterface> Engine::GraphicInterfaceAccessor::s_graphic_interface_ = {};