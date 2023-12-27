#pragma once

// Need to be included before boost only in the header, requires a default
// constructor
#define SERIALIZER_ACCESS                                                      \
  friend class Engine::Serializer;                                             \
  friend class boost::serialization::access;                                   \
  template <class Archive>                                                     \
  void serialize(Archive &ar, const unsigned int file_version);
// part of serialization access implementation, forward declaration of serialize
// function
#define SERIALIZER_ACCESS_IMPL1(NAMESPACE_TYPE)                                \
  template void NAMESPACE_TYPE::serialize<boost::archive::text_iarchive>(      \
      boost::archive::text_iarchive & ar, const unsigned int file_version);    \
  template void NAMESPACE_TYPE::serialize<boost::archive::text_oarchive>(      \
      boost::archive::text_oarchive & ar, const unsigned int file_version);    \
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
#define SERIALIZER_ACCESS_IMPL(NAMESPACE_TYPE, ...)                            \
  SERIALIZER_ACCESS_IMPL1(NAMESPACE_TYPE)                                      \
  SERIALIZER_ACCESS_IMPL2(NAMESPACE_TYPE, __VA_ARGS__)

// Static Component type, this should be added to every component
#define INTERNAL_COMP_CHECK_CONSTEXPR(enum_val) static constexpr eComponentType ctype = enum_val;
// Static Resource type, this should be added to every resource
#define INTERNAL_RES_CHECK_CONSTEXPR(enum_val) static constexpr eResourceType rtype = enum_val;
// Static engine default provided object type, this should be added to every object
#define INTERNAL_OBJECT_CHECK_CONSTEXPR(enum_val) static constexpr eDefObjectType dotype = enum_val;
// Static constant buffer type, this should be added to every constant buffer
#define INTERNAL_CB_CHECK_CONSTEXPR(enum_val) static constexpr eCBType cbtype = enum_val;

// Static client provided scene type, this should be added to every scene in the client
#define CLIENT_SCENE_CHECK_CONSTEXPR(enum_val) static constexpr Engine::eSceneType stype = enum_val;

// Static inline resource getter which infers self as type
#define RESOURCE_SELF_INFER_GETTER(TYPE)                                      \
  static inline boost::weak_ptr<TYPE> Get(const std::string& name)            \
    { return Engine::Manager::ResourceManager::GetInstance()                  \
       .GetResource<TYPE>(name); }

// Creatable resource creator which infers self as type
#define RESOURCE_SELF_INFER_CREATE(TYPE)                                      \
    static inline boost::shared_ptr<TYPE> Create(                             \
    const std::string& name, const std::filesystem::path& path)               \
    {                                                                         \
        if (const auto check = GetResourceManager().                          \
                               GetResourceByPath<TYPE>(path).lock())          \
        {                                                                     \
            return check;                                                     \
        }                                                                     \
        if (const auto check = GetResourceManager().                          \
                               GetResource<TYPE>(name).lock())                \
        {                                                                     \
            return check;                                                     \
        }                                                                     \
        const auto obj = boost::make_shared<TYPE>(path);                      \
        GetResourceManager().AddResource(name, obj);                          \
        return obj;                                                           \
    }


// Invalid id check
#define INVALID_ID_CHECK(ID) ID != g_invalid_id

// invalid id check for weak pointer
#define INVALID_ID_CHECK_WEAK_RETURN(ID)                                      \
  if ((ID) == g_invalid_id) return {};