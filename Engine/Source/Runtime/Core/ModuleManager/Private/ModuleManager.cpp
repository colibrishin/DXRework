#include "ModuleManager/Public/ModuleManager.h"
#include "ModuleManager.generated.h"

#include <ranges>

namespace Engine::Managers
{
	ModuleManager::ModuleManager(SINGLETON_LOCK_TOKEN) {}

	ENGINE_CORE_API void ModuleManager::Initialize()
	{
		m_module_paths_.emplace(L"Default", "./");
	}

	ENGINE_CORE_API ModuleManager::ModuleInfo* ModuleManager::FindModule(const std::wstring_view name)
	{
		std::lock_guard l(m_critical_mutex_);
		if (!m_module_loaded_.contains(name.data()))
		{
			return nullptr;
		}

		return m_module_loaded_.at(name.data()).get();
	}

	ENGINE_CORE_API IModule* ModuleManager::LoadModule(const std::wstring_view name)
	{
		ModuleInfo* module_info = FindModule(name);

		if (module_info)
		{
			if (IModule* module = module_info->m_module_.get())
			{
				return module;
			}
		}
		else
		{
			AddModule(name);
			module_info = FindModule(name);
		}

		std::lock_guard l(m_critical_mutex_);
#if !IS_DLL
		// Static Library
		if (m_module_initializer_.contains(name.data()))
		{
			if (const ModuleInitializationFunction& func = m_module_initializer_.at(name.data()))
			{
				module_info->m_module_ = std::unique_ptr<IModule>(func());
				module_info->m_module_->Initialize();
				return module_info->m_module_.get();
			}
			else
			{
				return nullptr;
			}
		}
#else
		// DLL
		{
			if (const HMODULE hModule = GetModuleHandleW(module_info->m_path_.c_str()))
			{
				module_info->m_handle_ = hModule;
				module_info->m_b_dynamic_ = false;
			}
			else
			{
				module_info->m_handle_ = LoadLibraryW(module_info->m_path_.c_str());
				module_info->m_b_dynamic_ = true;
			}

			if (!module_info->m_handle_)
			{
				module_info->m_last_error_ = GetLastError();
				return nullptr;
			}

			const ModuleInitializationFunctionCStyle& init_func = (ModuleInitializationFunctionCStyle)GetProcAddress(static_cast<HMODULE>(module_info->m_handle_), "InitializeModule");

			if (init_func)
			{
				module_info->m_module_ = std::unique_ptr<IModule>(init_func());
				module_info->m_module_->Initialize();
				return module_info->m_module_.get();
			}
			else
			{
				FreeLibrary(static_cast<HMODULE>(module_info->m_handle_));
				return nullptr;
			}
		}
#endif
	}

#if !IS_DLL
	void ModuleManager::RegisterStaticModule(const std::wstring_view name, const ModuleInitializationFunction& func)
	{
		m_module_initializer_.emplace(name, func);
	}
#endif

	ENGINE_CORE_API void ModuleManager::AddModule(const std::wstring_view name)
	{
		if (m_module_loaded_.contains(name.data()))
		{
			return;
		}

		std::lock_guard l(m_critical_mutex_);
		m_module_loaded_.emplace(name, std::make_unique<ModuleInfo>());

		ModuleInfo* module_info = m_module_loaded_.at(name.data()).get();
		module_info->m_filename_ = name;

		bool found = false;
#if IS_DLL
		module_info->m_filename_ext_ = std::wstring(name) + L".dll";

		for (const auto& path : m_module_paths_ | std::views::values)
		{
			if (found)
			{
				break;
			}

			if (std::filesystem::is_directory(path))
			{
				for (const std::filesystem::directory_entry& it : std::filesystem::directory_iterator(path))
				{
					if (it.path().extension() == L".dll" && it.path().stem() == name)
					{
						module_info->m_path_ = std::filesystem::absolute(it.path());
						found = true;
						break;
					}
				}
			}
		}
#else
		found = true;
#endif
	}

	ModuleManager::~ModuleManager()
	{
		for (auto& ptr : m_module_loaded_ | std::views::reverse | std::views::values)
		{
			if (ptr->m_module_)
			{
				ptr->m_module_.reset();
			}
			
			if (ptr->m_handle_)
			{
				FreeLibrary(static_cast<HMODULE>(ptr->m_handle_));
			}

			ptr.reset();
		}
	}
	
	void ModuleManager::PreUpdate(const float dt) {}
	void ModuleManager::FixedUpdate(const float dt) {}
	void ModuleManager::Update(const float dt) {}
	void ModuleManager::PreRender(const float dt) {}
	void ModuleManager::Render(const float dt) {}
	void ModuleManager::PostRender(const float dt) {}
	void ModuleManager::PostUpdate(const float dt) {}
}
