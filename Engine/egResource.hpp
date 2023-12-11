#pragma once
#include <filesystem>
#include "egCommon.hpp"
#include "egRenderable.hpp"

namespace Engine::Abstract
{
	class Resource : public Renderable
	{
	public:
		~Resource() override;

		virtual void Load() final;
		void Unload();

		void OnDeserialized() override;

		bool IsLoaded() const
		{
			return m_bLoaded_;
		}

		const std::filesystem::path& GetPath() const
		{
			return m_path_;
		}

		void SetPath(const std::filesystem::path& path)
		{
			m_path_ = std::move(path);
			m_path_str_ = m_path_.generic_string();
		}

		eResourcePriority GetPriority() const
		{
			return m_priority_;
		}

		void OnImGui() override;
		TypeName GetVirtualTypeName() const override;

	protected:
		Resource(std::filesystem::path path, eResourcePriority priority);

		virtual void Load_INTERNAL() = 0;
		virtual void Unload_INTERNAL() = 0;

	private:
		SERIALIZER_ACCESS

	private:
		bool m_bLoaded_;
		std::filesystem::path m_path_;
		std::string m_path_str_; // for serialization
		eResourcePriority m_priority_;

	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Abstract::Resource)