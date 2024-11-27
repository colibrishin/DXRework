#pragma once
#include "Source/Runtime/Abstracts/CoreEntity/Public/Entity.hpp"

namespace Engine::Abstracts
{
	class Resource : public Entity
	{
	public:
		using type = Resource;
		~Resource() override;

		virtual void Load() final;
		void         Unload();

		void OnDeserialized() override;

		[[nodiscard]] bool IsLoaded() const;
		[[nodiscard]] const std::filesystem::path&        GetPath() const;
		[[nodiscard]] virtual eResourceType GetResourceType() const;

		void SetPath(const std::filesystem::path& path);
		
	protected:
		Resource(std::filesystem::path path, eResourceType type);

		virtual void Load_INTERNAL() = 0;
		virtual void Unload_INTERNAL() = 0;

	private:
		Resource();
		SERIALIZE_DECL
		friend class Managers::ResourceManager;

		bool          m_bLoaded_;
		std::string   m_path_str_; // for serialization
		eResourceType m_type_;

		// non-serialized
		std::filesystem::path m_path_;
	};
} // namespace Engine::Abstract

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Engine::Abstract::Resource)
