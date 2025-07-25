#include "Allocator.h"

std::unordered_map<size_t, const Engine::alloc_base*> g_static_alloc = {};
