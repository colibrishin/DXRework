#pragma once
#include <boost/smart_ptr.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/access.hpp>

#include <boost/serialization/vector.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include "CoreEntity.h"

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
#define SERIALIZER_ACCESS_IMPL1(API, NAMESPACE_TYPE)                                  \
  template API void NAMESPACE_TYPE::serialize<boost::archive::binary_iarchive>(      \
      boost::archive::binary_iarchive & ar, const unsigned int file_version);    \
  template API void NAMESPACE_TYPE::serialize<boost::archive::binary_oarchive>(      \
      boost::archive::binary_oarchive & ar, const unsigned int file_version);    \
    BOOST_CLASS_EXPORT_IMPLEMENT(NAMESPACE_TYPE)

// serialization macros
#define _ARTAG(TYPENAME) ar & TYPENAME;
// serialization macros, requires if object is inherited from another object
#define _BSTSUPER(BASE) _ARTAG(boost::serialization::base_object<BASE>(*this))

// part of serialization access implementation, serialize function
// implementation
#define SERIALIZER_ACCESS_IMPL2(API, NAMESPACE_TYPE, ...)                           \
  template <class Archive>                                                     \
  void NAMESPACE_TYPE::serialize(Archive &ar,                                  \
                                 const unsigned int file_version) {            \
    __VA_ARGS__                                                                \
  }
// full serialization access implementation for a class only in the cpp file,
// requires a boost include
#define SERIALIZE_IMPL(API, NAMESPACE_TYPE, ...)                                    \
  SERIALIZER_ACCESS_IMPL1(API, NAMESPACE_TYPE)                                      \
  SERIALIZER_ACCESS_IMPL2(API, NAMESPACE_TYPE, __VA_ARGS__)

#if WITH_EDITOR
#include <UIInterface.h>
#endif

POLYMORPHIC_TYPE_MAP(Engine::Abstracts::Entity, void)

namespace Engine::Abstracts
{
	class ENGINE_COREENTITY_API Entity : public boost::enable_shared_from_this<Entity>
	{
	public:
		static std::string_view StaticTypeName()
		{
			return static_type_name<Entity>::name();
		}

		static std::string_view StaticFullTypeName()
		{
			return static_type_name<Entity>::full_name();
		}

		static HashType StaticTypeHash()
		{
			return type_hash<Entity>::value;
		}

		static bool StaticIsBaseOf(HashType hash)
		{
			return polymorphic_type_hash<Entity>::is_base_of(hash);
		}

		Entity(const Entity& other) : enable_shared_from_this(other)
		{
			m_name_ = other.m_name_;
#if WITH_EDITOR
			m_ui_info_ = other.m_ui_info_;
			m_precached_id_ = GetID();
#endif
			m_b_initialized_ = false;
			m_b_garbage_ = false;
			m_meta_path_ = other.m_meta_path_;
		}

		virtual ~Entity()           = default;

		bool operator==(const Entity& other) const
		{
			return GetID() == other.GetID();
		}

		virtual void SetName(const EntityName& name);
#if WITH_EDITOR
		virtual void OnNameChanged();
#endif
		void SetGarbage(bool garbage);

		const std::filesystem::path& GetMetadataPath() const;
		GlobalEntityID               GetID() const;
		const EntityName&            GetName() const;
		virtual TypeName             GetTypeName() const;
		virtual TypeName             GetPrettyTypeName() const;
		virtual HashType             GetTypeHash() const;
		virtual bool                 IsBaseOf(HashType hash) const;
		bool                         IsGarbage() const;
		bool                         IsInitialized() const;

		template <typename T>
		Weak<T> GetWeakPtr()
		{
			return boost::reinterpret_pointer_cast<T>(shared_from_this());
		}

		template <typename T>
		Strong<T> GetSharedPtr()
		{
			return boost::reinterpret_pointer_cast<T>(shared_from_this());
		}

		virtual void Initialize();
		virtual void PreUpdate(const float dt) = 0;
		virtual void Update(const float dt) = 0;
		virtual void PostUpdate(const float dt) = 0;
		virtual void FixedUpdate(const float dt) = 0;
		virtual void OnUIUpdate(UIContext* const parent, const float dt);

		virtual void OnSerialized() = 0;
		virtual void OnDeserialized() = 0;

	protected:
		Entity() :
#if WITH_EDITOR
			m_precached_id_(GetID()),
#endif
			m_b_initialized_(false),
			m_b_garbage_(false) {}

	private:
		SERIALIZE_DECL

		EntityName     m_name_;
#if WITH_EDITOR
	public:
		UIInfo         m_ui_info_;
	private:
		GlobalEntityID m_precached_id_;
#endif
		bool		   m_b_initialized_;
		bool           m_b_garbage_;

		std::filesystem::path m_meta_path_;
	};
} // namespace Engine::Abstracts

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Engine::Abstracts::Entity)