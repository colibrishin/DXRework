#pragma once
#include <boost/smart_ptr.hpp>
#include <filesystem>
#include "CoreEntity.h"

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
			m_b_initialized_(false),
			m_b_garbage_(false),
			m_precached_id_(GetID()) {}

	private:
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