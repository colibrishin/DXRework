#pragma once
#include <string>
#include <filesystem>
#include <memory>
#include "IModule.h"

namespace Engine
{
    struct ENGINE_COREMODULE_API ModuleInfo
    {
        std::wstring          m_filename_ext_;
        std::wstring          m_filename_;
        std::filesystem::path m_path_;

        void*    m_handle_     = nullptr;
        bool     m_b_dynamic_  = false;
        bool     m_b_lazy      = false;
        uint64_t m_last_error_ = 0;

        std::unique_ptr<IModule> m_module_;

        ~ModuleInfo();
    };
}