#include "ModuleInfo.h"
#include "Macro.h"

#if PLATFORM == Windows
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
