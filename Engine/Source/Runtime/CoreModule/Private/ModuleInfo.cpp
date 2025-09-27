#include "ModuleInfo.h"
#include "Macro.h"

#if _WIN32 || _WIN64
#include <Windows.h>
#endif

namespace Engine
{
    ModuleInfo::~ModuleInfo()
    {
#if IS_DLL
        if ( !GetModuleHandleW( m_path_.c_str() ) )
        {
            m_module_.release();
        }
#endif
    }
}
