#pragma once
#include "CoreType.h"

#include "Serialization.hpp"
#include "IModule.generated.h"

namespace Engine
{
	ECLASS(abstract)
	struct ENGINE_CORE_API IModule
	{
		GENERATE_BODY
		virtual ~IModule() = default;

		void Initialize()
		{
			if (InitializeImpl()) 
			{
				m_b_is_initialized_ = true;
			}
		}
		void Shutdown()
		{
			if (ShutdownImpl()) 
			{
				m_b_is_initialized_ = false;
			}
		}

		virtual bool InitializeImpl() = 0;
		virtual bool ShutdownImpl() = 0;
		virtual bool DynamicLoadable() = 0;

		virtual const std::vector<std::string>& GetDependencies() const = 0;
		virtual const std::vector<std::string>& LoadAfter() const
		{
			static std::vector<std::string> load_after = {};
			return load_after;
		}

	private:
		bool m_b_is_initialized_ = false;
	};
}