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
			m_path_wstr_ = m_path_.generic_wstring();
		}

		eResourcePriority GetPriority() const
		{
			return m_priority_;
		}

	protected:
		Resource(std::filesystem::path path, eResourcePriority priority);

		virtual void Load_INTERNAL() = 0;
		virtual void Unload_INTERNAL() = 0;
		void AfterDeserialized() override;

	private:
		SERIALIZER_ACCESS

		bool m_bLoaded_;
		std::filesystem::path m_path_;
		std::wstring m_path_wstr_; // for serialization
		eResourcePriority m_priority_;

	};
}
