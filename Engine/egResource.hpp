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
		}

		eResourcePriority GetPriority() const
		{
			return m_priority_;
		}

	protected:
		Resource(std::filesystem::path path, eResourcePriority priority) : m_bLoaded_(false), m_path_(std::move(path)), m_priority_(priority)
		{
		}

		virtual void Load_INTERNAL() = 0;
		virtual void Unload_INTERNAL() = 0;

	private:
		bool m_bLoaded_;
		std::filesystem::path m_path_;
		eResourcePriority m_priority_;

	};

	inline Resource::~Resource()
	{
		Unload();
	}

	inline void Resource::Load()
	{
		if (!m_bLoaded_)
		{
			Load_INTERNAL();
			m_bLoaded_ = true;
		}
	}

	inline void Resource::Unload()
	{
		if (m_bLoaded_)
		{
			Unload_INTERNAL();
			m_bLoaded_ = false;
		}
	}
}
