#pragma once
#include <filesystem>
#include <functional>

#include "boost/preprocessor/facilities/is_empty.hpp"
#include "Source/Runtime/Core/ModuleManager/Public/IModule.h"
#include "Source/Runtime/CoreSingleton/Public/Singleton.hpp"

#define IS_EMPTY(...) (true __VA_OPT__(&& false))
#define IS_DLL !BOOST_PP_IS_EMPTY(ENGINE_CORE_API)

namespace Engine::Managers
{
	class ModuleManager;
}

POLYMORPHIC_MANAGER_TYPE_MAP(ENGINE_CORE_API, Engine::Managers::ModuleManager)

namespace Engine::Managers
{
	using ModuleInitializationFunctionCStyle = IModule*(*)();
	using ModuleInitializationFunction = std::function<IModule*()>;

	class ENGINE_CORE_API ModuleManager : public Abstracts::Singleton<ModuleManager>
	{
	public:
		explicit ModuleManager(SINGLETON_LOCK_TOKEN);

	private:
		struct ModuleInfo
		{
			std::wstring m_filename_ext_;
			std::wstring m_filename_;
			std::filesystem::path m_path_;

			void* m_handle_;
			bool m_b_dynamic_ = false;
			uint64_t m_last_error_ = 0;
			
			std::unique_ptr<IModule> m_module_;
		};

		using ModuleInfoPtr = std::unique_ptr<ModuleInfo>;
		using ModuleMap = std::unordered_map<std::wstring, ModuleInfoPtr>;

	public:
		void        Initialize() override;
		ModuleInfo* FindModule(const std::wstring_view name);
		IModule*    LoadModule(const std::wstring_view name);
		void        AddModule(const std::wstring_view name);

#if !IS_DLL
		void                 RegisterStaticModule(const std::wstring_view name, const ModuleInitializationFunction& func);
#endif

	private:
		friend struct SingletonDeleter;
		~ModuleManager() override;

		void PreUpdate(const float dt) override;
		void FixedUpdate(const float dt) override;
		void Update(const float dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void PostRender(const float dt) override;
		void PostUpdate(const float dt) override;

		std::mutex                                                     m_critical_mutex_;
		std::unordered_map<std::wstring, ModuleInfoPtr>				   m_module_loaded_{};
		std::unordered_map<std::wstring, ModuleInitializationFunction> m_module_initializer_{};
		std::unordered_map<std::wstring, std::filesystem::path>        m_module_paths_{};
	};

#if !IS_DLL
	template <typename ModuleType>
	struct StaticLinkModuleEntry
	{
		explicit StaticLinkModuleEntry(const std::wstring_view name)
		{
			ModuleManager::RegisterStaticModule(name, &StaticLinkModuleEntry<ModuleType>::InitializeModule);
		}

		static IModule* InitializeModule()
		{
			return new ModuleType();
		}
	};
#endif
}

#if !IS_DLL
#define MODULE_IMPL(ModuleType, Name) \
	static StaticLinkModuleEntry<ModuleType> ModuleEntry##Name(L"#Name"); \
	extern "C" void MODULE_IMPL_#Name() {} 
#else
#define MODULE_IMPL(ModuleType, Name) \
	extern "C" DLLEXPORT Engine::IModule* InitializeModule() \
	{ \
		return new ModuleType(); \
	} \
	extern "C" void MODULE_IMPL_##Name() {}
#endif