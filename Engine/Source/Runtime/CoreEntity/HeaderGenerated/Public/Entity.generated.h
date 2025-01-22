#ifdef ENTITY_GENERATED_H
#ifndef POST_ENTITY_H
#define POST_ENTITY_H
BOOST_CLASS_EXPORT_IMPLEMENT(Engine::Abstracts::Entity)

#endif
#endif
#ifndef ENTITY_GENERATED_H
#define ENTITY_GENERATED_H
#include "../Misc.h"
#include <array>
#include <algorithm>
#include <boost/serialization/export.hpp>
#include <boost/serialization/access.hpp>
#ifdef GENERATE_BODY
#undef GENERATE_BODY
#endif
#define GENERATE_BODY public: typedef void Base;static std::string_view StaticTypeName(){return static_type_name<Engine::Abstracts::Entity>::name();}static std::string_view StaticFullTypeName(){return static_type_name<Engine::Abstracts::Entity>::full_name();}static HashType StaticTypeHash(){return &type_hash<Engine::Abstracts::Entity>::value;}static bool StaticIsBaseOf(HashType hash){return polymorphic_type_hash<Engine::Abstracts::Entity>::is_base_of(hash);} virtual std::string_view GetTypeName() const { return Entity::StaticFullTypeName(); }virtual std::string_view GetPrettyTypeName() const { return Entity::StaticTypeName(); }virtual HashType GetTypeHash() const { return Entity::StaticTypeHash(); }virtual bool IsBaseOf(HashType hash) const { return Entity::StaticIsBaseOf(hash); } friend class Engine::Serializer; friend class boost::serialization::access; private: template <class Archive> void serialize(Archive &ar, const unsigned int file_version) {ar& m_name_; ar& m_meta_path_; } public: 
namespace Engine::Abstracts {class Entity;}
BOOST_SERIALIZATION_ASSUME_ABSTRACT(Engine::Abstracts::Entity)
template <> struct polymorphic_type_hash<Engine::Abstracts::Entity>{static constexpr size_t upcast_count = 1 + polymorphic_type_hash<void>::upcast_count;static constexpr auto upcast_array = []{HashArray<upcast_count> ret{&type_hash<Engine::Abstracts::Entity>::value};std::copy_n(polymorphic_type_hash<void>::upcast_array.begin(),  polymorphic_type_hash<void>::upcast_array.size(), ret.data() + 1);std::ranges::sort(ret, [](const auto lhs, const auto rhs) {return *lhs < *rhs;});return ret;}();static bool is_base_of(const HashType hash) { if constexpr ((upcast_count * sizeof(HashTypeValue)) < (1 << 7)) { return std::ranges::find_if(upcast_array, [&hash](const auto other){return hash->Equal(*other);}) != upcast_array.end(); } return std::ranges::binary_search(upcast_array, hash, [](const auto lhs, const auto rhs){return *lhs < *rhs;}); } }; 
#endif
