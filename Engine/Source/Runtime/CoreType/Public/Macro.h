#pragma once

#include <chrono>

#define EMPTY
#ifndef DLLIMPORT
#define DLLIMPORT __declspec( dllimport )
#endif

#ifndef DLLEXPORT
#define DLLEXPORT __declspec( dllexport )
#endif

#define ECLASS( ... )
#define EENUM( ... )
#define EFUNC( ... )
#define EPROPERTY( ... )
#define GENERATE_BODY

#define STRINGIFY( X )      STRINGIFY_IMPL( X )
#define STRINGIFY_IMPL( X ) #X

#include "boost/preprocessor/facilities/is_empty.hpp"

#define IS_DLL !BOOST_PP_IS_EMPTY( ENGINE_CORE_API )

#ifdef _UNICODE
#ifndef UNICODE
#define UNICODE
#endif
#endif

#define WIDEN2( x )         L##x
#define WIDEN( x )          WIDEN2( x )
#define STRINGIFY( X )      STRINGIFY_IMPL( X )
#define STRINGIFY_IMPL( X ) #X

#if SERVER
#define CONSOLE_OUT( PREFIX, FMT, ... )                                                                                \
    std::cout << std::format( "[{} | {}]: ",                                                                           \
                              std::chrono::duration_cast<std::chrono::nanoseconds>(                                    \
                                      std::chrono::high_resolution_clock::now().time_since_epoch() ),                  \
                              PREFIX );                                                                                \
    std::cout << std::format( FMT, __VA_ARGS__ ) << '\n';
#else
#define CONSOLE_OUT( PREFIX, FMT, ... )
#endif
