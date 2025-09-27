#include "Allocator.h"

std::unordered_map<size_t, Engine::alloc_base*> g_static_alloc = {};
