#pragma once
#include "GraphicInterface.h"
#include "Singleton.hpp"

#include "ModuleManager/Public/IModule.h"

#include "Source/Runtime/CoreUI/Public/UIInterface.h"

namespace Engine
{
    void AlignText(const std::string_view label);
    [[nodiscard]] std::string LabelSuffix(const std::string_view label);

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
	    ImGuiDialogToken(const void* context, const std::string_view title, bool& opened)
		    : DialogToken(context, title, opened) {}

	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(const void*, const std::string_view, bool&) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiButtonToken : ButtonToken
    {
	    explicit ImGuiButtonToken(const std::string_view title)
		    : ButtonToken(title) {}

	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(const std::string_view) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiLabelAndTextToken : LabelAndTextToken
    {
	    ImGuiLabelAndTextToken(const std::string_view title, std::string& target, bool editable)
		    : LabelAndTextToken(title, target, editable) {}

	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(const std::string_view, std::string&, bool) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiLabelAndFloatToken : LabelAndFloatToken
    {
	    ImGuiLabelAndFloatToken(const std::string_view title, float& target, float step, float speed, bool editable)
		    : LabelAndFloatToken(title, target, step, speed, editable) {}

	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(const std::string_view, float&, float, float, bool) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiLabelAndIntToken : LabelAndIntToken
    {
	    ImGuiLabelAndIntToken(const std::string_view title, int& target, bool editable)
		    : LabelAndIntToken(title, target, editable) {}

	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(const std::string_view, int&, bool) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiLabelAndUIntToken : LabelAndUIntToken
    {
	    ImGuiLabelAndUIntToken(const std::string_view title, uint32_t& target, bool editable)
		    : LabelAndUIntToken(title, target, editable) {}

	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(const std::string_view, unsigned int&, bool) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiLabelAndULLDToken : LabelAndULLDToken
    {
	    ImGuiLabelAndULLDToken(const std::string_view title, uint64_t& target, bool editable)
		    : LabelAndULLDToken(title, target, editable) {}

	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(const std::string_view, unsigned long long&, bool) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiLabelAndPathToken : LabelAndPathToken
    {
	    ImGuiLabelAndPathToken(const std::string_view title, const std::filesystem::path& path)
		    : LabelAndPathToken(title, path) {}

	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(const std::string_view, const std::filesystem::path&) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiListBoxToken : ListBoxToken
    {
	    ImGuiListBoxToken(const std::string_view label, float x, float y)
		    : ListBoxToken(label, x, y) {}

	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(const std::string_view, float, float) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiTreeNodeToken : TreeNodeToken
    {
	    explicit ImGuiTreeNodeToken(const std::string_view label)
		    : TreeNodeToken(label) {}

	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(const std::string_view) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiSelectableToken : SelectableToken
    {
	    ImGuiSelectableToken(const std::string_view label, bool& opened)
		    : SelectableToken(label, opened) {}

	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(const std::string_view, bool&) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiLabelAndVec3Token : LabelAndVec3Token 
    {
        ImGuiLabelAndVec3Token(const std::string_view label, float& vec)
            : LabelAndVec3Token(label, vec) {}
        void End() const override;

    protected:
        [[nodiscard]] bool DoImpl(const std::string_view, float&) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiCheckboxToken : CheckboxToken
    {
        ImGuiCheckboxToken(const std::string_view label, bool& flag)
            : CheckboxToken(label, flag) {}

        void End() const override;

    protected:
        [[nodiscard]] bool DoImpl(const std::string_view, bool&) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiComboboxToken : ComboboxToken
    {
        ImGuiComboboxToken(const std::string_view label, int* value, const char* const* label_arr, const size_t arr_size)
            : ComboboxToken(label, value, label_arr, arr_size) {}

        void End() const override;

    protected:
        [[nodiscard]] bool DoImpl(const std::string_view label, int* value, const char* const* label_arr, const size_t arr_size) const override;
    };

#define IMGUI_INLINE_GETTER_DECL(Name) \
    Name##Token* New##Name##(const Name##Token::ArgumentTuple& arguments) override \
    { \
		return Generate<ImGui##Name##Token>(arguments); \
    }

    struct ENGINE_IMGUIMANAGER_API ImGuiUIInterface final : UIInterface
    {
        IMGUI_INLINE_GETTER_DECL(MainMenuBar)
        IMGUI_INLINE_GETTER_DECL(Menu)
        IMGUI_INLINE_GETTER_DECL(MenuItem)
        IMGUI_INLINE_GETTER_DECL(Dialog)
        IMGUI_INLINE_GETTER_DECL(Button)
        IMGUI_INLINE_GETTER_DECL(LabelAndText)
        IMGUI_INLINE_GETTER_DECL(LabelAndFloat)
        IMGUI_INLINE_GETTER_DECL(LabelAndInt)
        IMGUI_INLINE_GETTER_DECL(LabelAndUInt)
        IMGUI_INLINE_GETTER_DECL(LabelAndULLD)
        IMGUI_INLINE_GETTER_DECL(LabelAndPath)
        IMGUI_INLINE_GETTER_DECL(ListBox)
        IMGUI_INLINE_GETTER_DECL(TreeNode)
        IMGUI_INLINE_GETTER_DECL(Selectable)
        IMGUI_INLINE_GETTER_DECL(LabelAndVec3)
        IMGUI_INLINE_GETTER_DECL(Checkbox)
        IMGUI_INLINE_GETTER_DECL(Combobox)

        void               NewFrame() override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiManagerModule : public IModule
    {
        INLINE_COMPILE_TIME_TYPENAME(ImGuiManagerModule)
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
        INLINE_COMPILE_TIME_TYPENAME(ImGuiManager)
        explicit ImGuiManager(SINGLETON_LOCK_TOKEN) {}

        void Initialize() override;

        void OnUIUpdate(UIContext* const parent, const float dt) override;
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
