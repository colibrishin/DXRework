#pragma once
#include "GraphicInterface.h"
#include "Singleton.hpp"

#include "ModuleManager/Public/IModule.h"

#include "Source/Runtime/CoreUI/Public/UIInterface.h"

namespace Engine
{
    void AlignText(const std::string_view label);
    [[nodiscard]] std::string LabelPrefix(const std::string_view label);

    struct ENGINE_IMGUIMANAGER_API ImGuiMainMenuBarToken : MainMenuBarToken
    {
        void               End() const override;
        [[nodiscard]] bool DoImpl() const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiMenuToken : MenuToken
    {
        explicit ImGuiMenuToken(const std::string_view title)
            : MenuToken(title) {}

        void End() const override;
        [[nodiscard]] bool              DoImpl(const std::string_view) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiMenuItemToken : MenuItemToken
    {
        explicit ImGuiMenuItemToken(const std::string_view title)
            : MenuItemToken(title) {}

        void End() const override;
        [[nodiscard]] bool                  DoImpl(const std::string_view) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiDialogToken : DialogToken
    {
	    ImGuiDialogToken(const std::string_view title, bool& opened)
		    : DialogToken(title, opened) {}

	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(const std::string_view, bool&) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiUIInterface final : UIInterface
    {
        MainMenuBarToken* NewMainMenuBar(const MainMenuBarToken::ArgumentTuple& arguments) override
        {
            return Generate<ImGuiMainMenuBarToken>(arguments);
        }
        
        MenuToken* NewMenu(const MenuToken::ArgumentTuple& arguments) override
        {
            return Generate<ImGuiMenuToken>(arguments);
        }
        
        MenuItemToken* NewMenuItem(const MenuItemToken::ArgumentTuple& arguments) override
        {
            return Generate<ImGuiMenuItemToken>(arguments);
        }

        DialogToken* NewDialog(const DialogToken::ArgumentTuple& arguments) override
        {
	        return Generate<ImGuiDialogToken>(arguments);
        }

        void NewFrame() override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiManagerModule : public IModule
    {
        void Initialize() override;
        void Shutdown() override;
        bool DynamicLoadable() override;
    };
}

namespace Engine::Managers
{
    class ENGINE_IMGUIMANAGER_API ImGuiManager final : public Abstracts::Singleton<ImGuiManager>
    {
    public:
        explicit ImGuiManager(SINGLETON_LOCK_TOKEN) {}

        void Initialize() override;

        void OnUIUpdate(const float dt) override;
        void PreUpdate(const float dt) override;
        void Update(const float dt) override;
        void PreRender(const float dt) override;
        void Render(const float dt) override;
        void PostRender(const float dt) override;
        void FixedUpdate(const float dt) override;
        void PostUpdate(const float dt) override;

    private:
        friend struct SingletonDeleter;
        ~ImGuiManager() override;

        // ImGui Graphics
        Unique<GraphicHeapBase> m_imgui_descriptor_;
    };
} // namespace Engine::Managers
