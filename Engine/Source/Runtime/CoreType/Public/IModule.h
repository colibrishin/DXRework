#pragma once
#include <vector>
#include <string>
#include "CoreType.h"
#include "boost/preprocessor/facilities/is_empty.hpp"

#include "IModule.generated.h"

#ifndef IS_DLL
#define IS_DLL !BOOST_PP_IS_EMPTY(ENGINE_CORETYPE_API)
#endif

namespace Engine
{
    ECLASS(abstract)
	struct ENGINE_CORETYPE_API IModule
	{
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

		virtual const std::vector<std::string>& GetDependencies() const
		{
			static const std::vector<std::string> empty = {};
			return empty;
		}

		virtual const std::vector<std::string>& LoadAfter() const
		{
			static std::vector<std::string> load_after = {};
			return load_after;
		}

	private:
		bool m_b_is_initialized_ = false;
	};
}

#if !IS_DLL
#define MODULE_IMPL(ModuleType, Name) \
	static StaticLinkModuleEntry<ModuleType> ModuleEntry##Name(L"##Name##"); \
	extern "C" void MODULE_IMPL_##Name() {} 
#else
#define MODULE_IMPL(ModuleType, Name) \
	extern "C" DLLEXPORT Engine::IModule* InitializeModule() \
	{ \
		return new ModuleType(); \
	} \
	extern "C" void MODULE_IMPL_##Name() {}
#endif