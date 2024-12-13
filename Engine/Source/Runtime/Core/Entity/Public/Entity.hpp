#pragma once
#include <filesystem>
#include <directx/d3d12.h>

#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

namespace Engine::Abstracts
{
	class CORE_API Entity : public boost::enable_shared_from_this<Entity>
	{
	public:
		Entity(const Entity& other) = default;
		virtual ~Entity()           = default;

		bool operator==(const Entity& other) const
		{
			return GetID() == other.GetID();
		}

		void SetName(const EntityName& name);
		void SetGarbage(bool garbage);

		const std::filesystem::path& GetMetadataPath() const;
		GlobalEntityID               GetID() const;
		EntityName                   GetName() const;
		TypeName                     GetTypeName() const;
		virtual TypeName             GetPrettyTypeName() const;
		bool                         IsGarbage() const;
		bool                         IsInitialized() const;

		template <typename T>
		__forceinline boost::weak_ptr<T> GetWeakPtr()
		{
			return boost::reinterpret_pointer_cast<T>(shared_from_this());
		}

		template <typename T>
		__forceinline boost::shared_ptr<T> GetSharedPtr()
		{
			return boost::reinterpret_pointer_cast<T>(shared_from_this());
		}

		virtual void Initialize();
		virtual void PreUpdate(const float& dt) = 0;
		virtual void Update(const float& dt) = 0;
		virtual void PostUpdate(const float& dt) = 0;
		virtual void FixedUpdate(const float& dt) = 0;

		virtual void OnSerialized() = 0;
		virtual void OnDeserialized() = 0;

	protected:
		Entity()
			: m_b_initialized_(false),
			  m_b_garbage_(false) {}

	private:
		EntityName m_name_;
		bool       m_b_initialized_;
		bool       m_b_garbage_;

		std::filesystem::path m_meta_path_;
	};
} // namespace Engine::Abstracts
