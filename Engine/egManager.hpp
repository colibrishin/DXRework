#pragma once
#include "egEntity.hpp"

namespace Engine::Abstract
{
	class Manager
	{
	public:
		virtual ~Manager() = default;

		virtual void Initialize() = 0;
		virtual void PreUpdate() = 0;
		virtual void Update() = 0;
		virtual void PreRender() = 0;
		virtual void Render() = 0;

		static Manager* GetManager()
		{
			return m_manager.get();
		}

	protected:
		Manager()
		{
			m_manager = std::unique_ptr<Manager>(this);
		}

		inline static std::unique_ptr<Manager> m_manager = nullptr;
	};
}
