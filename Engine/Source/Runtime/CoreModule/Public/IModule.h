#pragma once
#include <vector>
#include <string>
#include "Macro.h"

namespace Engine
{
    struct ENGINE_COREMODULE_API IModule
    {
        virtual ~IModule() = default;

        void Initialize()
        {
            if ( InitializeImpl() )
            {
                m_b_is_initialized_ = true;
            }
        }
        void Shutdown()
        {
            if ( ShutdownImpl() )
            {
                m_b_is_initialized_ = false;
            }
        }

        virtual bool InitializeImpl()  = 0;
        virtual bool ShutdownImpl()    = 0;
        virtual bool DynamicLoadable() = 0;

        virtual const std::vector<std::string>& GetDependencies() const
        {
            static const std::vector<std::string> empty = {};
            return empty;
        }

        virtual const std::vector<std::string>& LoadAfter() const
        {
            static const std::vector<std::string> load_after = {};
            return load_after;
        }

    private:
        bool m_b_is_initialized_ = false;
    };
} // namespace Engine
