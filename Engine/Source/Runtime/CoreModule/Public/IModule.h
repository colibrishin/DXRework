#pragma once
#include <string>
#include <vector>

#include "Macro.h"

#include "IModule.generated.h"

namespace Engine
{
    /// Load timing phase for module initialization order. Phase order: Core < Graphic < UI < Internal < Application.
    enum class ELoadPhase : int
    {
        Core       = 0,  // Memory, CoreModule, Core
        Graphic    = 1,  // D3D12GraphicInterface, RenderPipeline, Shader, Texture, Mesh, etc.
        UI         = 2,  // ImGuiManager, UI subsystems
        Internal   = 3,  // SceneManager, InputManager, CameraManager, SoundManager, PhysicsManager, etc.
        Application = 4  // Game/editor modules
    };

    ECLASS()
    struct ENGINE_COREMODULE_API IModule
    {
        GENERATE_BODY

        virtual ~IModule() = default;

        /// Called by ModuleManager before Initialize() when loaded as a DLL. Use GetModuleName() when
        /// registering with subsystems so they can perform full cleanup on UnregisterModule(module_name).
        void SetModuleName( std::wstring_view name )
        {
            m_module_name_.assign( name.begin(), name.end() );
        }
        std::wstring_view GetModuleName() const { return m_module_name_; }

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

        /// Load timing phase; used by ModuleManager to order initialization. Default Internal.
        virtual ELoadPhase GetLoadPhase() const
        {
            return ELoadPhase::Internal;
        }

    private:
        bool         m_b_is_initialized_ = false;
        std::wstring m_module_name_;
    };
} // namespace Engine
