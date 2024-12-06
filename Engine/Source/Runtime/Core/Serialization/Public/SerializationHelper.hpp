#pragma once

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/export.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/access.hpp>

namespace Engine
{
	class Serializer;
}

// Need to be included before boost only in the header, requires a default
// constructor
#define SERIALIZE_DECL                                                         \
  friend class Engine::Serializer;                                             \
  friend class boost::serialization::access;                                   \
  template <class Archive>                                                     \
  void serialize(Archive &ar, const unsigned int file_version);

// part of serialization access implementation, forward declaration of serialize
// function
#define SERIALIZER_ACCESS_IMPL1(NAMESPACE_TYPE)                                  \
  template void NAMESPACE_TYPE::serialize<boost::archive::binary_iarchive>(      \
      boost::archive::binary_iarchive & ar, const unsigned int file_version);    \
  template void NAMESPACE_TYPE::serialize<boost::archive::binary_oarchive>(      \
      boost::archive::binary_oarchive & ar, const unsigned int file_version);    \
    BOOST_CLASS_EXPORT_IMPLEMENT(NAMESPACE_TYPE)

// serialization macros
#define _ARTAG(TYPENAME) ar & TYPENAME;
// serialization macros, requires if object is inherited from another object
#define _BSTSUPER(BASE) boost::serialization::base_object<BASE>(*this)

// part of serialization access implementation, serialize function
// implementation
#define SERIALIZER_ACCESS_IMPL2(NAMESPACE_TYPE, ...)                           \
  template <class Archive>                                                     \
  void NAMESPACE_TYPE::serialize(Archive &ar,                                  \
                                 const unsigned int file_version) {            \
    __VA_ARGS__                                                                \
  }
// full serialization access implementation for a class only in the cpp file,
// requires a boost include
#define SERIALIZE_IMPL(NAMESPACE_TYPE, ...)                                    \
  SERIALIZER_ACCESS_IMPL1(NAMESPACE_TYPE)                                      \
  SERIALIZER_ACCESS_IMPL2(NAMESPACE_TYPE, __VA_ARGS__)