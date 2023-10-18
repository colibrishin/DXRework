#pragma once
#include <filesystem>

#include "egRenderable.hpp"

namespace Engine::Abstract
{
	class Resource : public Renderable
	{
	public:
		~Resource() override = default;

		virtual void Load() = 0;
		virtual void Unload() = 0;

		const std::filesystem::path& GetPath() const
		{
			return m_path_;
		}

	protected:
		Resource(std::filesystem::path path) : m_path_(std::move(path))
		{
		}

	private:
		std::filesystem::path m_path_;
	};
}
