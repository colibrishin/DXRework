#pragma once
#include "GraphicInterface.h"
#include "Singleton.h"

#include "Source/Runtime/CoreUI/Public/UIInterface.h"

#include "ImGuiManager.generated.h"

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
        using MenuToken::MenuToken;

        void End() const override;
        [[nodiscard]] bool              DoImpl(const std::string_view) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiMenuItemToken : MenuItemToken
    {
        using MenuItemToken::MenuItemToken;

        void End() const override;
        [[nodiscard]] bool                  DoImpl(const std::string_view) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiDialogToken : DialogToken
    {
        using DialogToken::DialogToken;

	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(const void*, const std::string_view, bool&) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiButtonToken : ButtonToken
    {
        using ButtonToken::ButtonToken;

	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(const std::string_view) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiLabelAndTextToken : LabelAndTextToken
    {
        using LabelAndTextToken::LabelAndTextToken;

	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(const std::string_view, std::string&, bool) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiLabelAndFloatToken : LabelAndFloatToken
    {
        using LabelAndFloatToken::LabelAndFloatToken;

	    void End() const override;

    protected:
        [[nodiscard]] bool DoImpl(const std::string_view, float&, float, float, float, bool) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiLabelAndIntToken : LabelAndIntToken
    {
        using LabelAndIntToken::LabelAndIntToken;

	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(const std::string_view, int&, float, int, int, bool) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiLabelAndUIntToken : LabelAndUIntToken
    {
        using LabelAndUIntToken::LabelAndUIntToken;

	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(const std::string_view, uint32_t&, float, uint32_t, uint32_t, bool) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiLabelAndULLDToken : LabelAndULLDToken
    {
        using LabelAndULLDToken::LabelAndULLDToken;

	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(const std::string_view, uint64_t&, float, uint64_t, uint64_t, bool) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiLabelAndPathToken : LabelAndPathToken
    {
        using LabelAndPathToken::LabelAndPathToken;

	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(const std::string_view, const std::filesystem::path&) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiListBoxToken : ListBoxToken
    {
        using ListBoxToken::ListBoxToken;

	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(const std::string_view, float, float) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiTreeNodeToken : TreeNodeToken
    {
        using TreeNodeToken::TreeNodeToken;

	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(const std::string_view) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiSelectableToken : SelectableToken
    {
        using SelectableToken::SelectableToken;

	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(const std::string_view, bool&) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiLabelAndVec3Token : LabelAndVec3Token 
    {
        using LabelAndVec3Token::LabelAndVec3Token;

        void End() const override;

    protected:
        [[nodiscard]] bool DoImpl(const std::string_view, float*, float, float, float, bool) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiCheckboxToken : CheckboxToken
    {
        using CheckboxToken::CheckboxToken;

        void End() const override;

    protected:
        [[nodiscard]] bool DoImpl(const std::string_view, bool&) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiComboboxToken : ComboboxToken
    {
        using ComboboxToken::ComboboxToken;

        void End() const override;

    protected:
        [[nodiscard]] bool DoImpl(const std::string_view label, int* value, const char* const* label_arr, const size_t arr_size) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiLabelAndVec4Token : LabelAndVec4Token
    {
        using LabelAndVec4Token::LabelAndVec4Token;

	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(const std::string_view, float*, float, float, float, bool) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiDragAndDropTargetToken : DragAndDropTargetToken
    {
        using DragAndDropTargetToken::DragAndDropTargetToken;
	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(const std::string_view, const std::function<void(void*)>) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiDragAndDropSourceToken : DragAndDropSourceToken
    {
        using DragAndDropSourceToken::DragAndDropSourceToken;
	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(const std::string_view, const std::string_view, const void*, unsigned long long) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiLabelAndUInt16Token : LabelAndUInt16Token
    {
        using LabelAndUInt16Token::LabelAndUInt16Token;

	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(
		    const std::string_view, unsigned short&, float, unsigned short, unsigned short, bool
	    ) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiComboboxUInt8Token : ComboboxUInt8Token
    {
        using ComboboxUInt8Token::ComboboxUInt8Token;
	    void End() const override;

    protected:
	    [[nodiscard]] bool DoImpl(
		    const std::string_view, unsigned char*, const char* const*, const unsigned long long
	    ) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiTextToken : TextToken 
    {
        using TextToken::TextToken;

        void End() const override;

    protected:
        bool DoImpl(const std::string_view text) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiSeparatorToken : SeparatorToken 
    {
        using SeparatorToken::SeparatorToken;

        void End() const override;

    protected:
        bool DoImpl() const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiTableToken : TableToken
    {
        using TableToken::TableToken;

        void End() const override;
    protected:
        bool DoImpl(std::string_view label, unsigned long long column_count) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiTableRowToken : TableRowToken
    {
        using TableRowToken::TableRowToken;
        
        void End() const override;
    protected:
        bool DoImpl() const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiTableColumnToken : TableColumnToken
    {
        using TableColumnToken::TableColumnToken;

        void End() const override;
    protected:
        bool DoImpl(unsigned long long column) const override;
    };

    struct ENGINE_IMGUIMANAGER_API ImGuiLabelAndVec2Token : LabelAndVec2Token
    {
        using LabelAndVec2Token::LabelAndVec2Token;

        void End() const override;

    protected:
        [[nodiscard]] bool DoImpl(const std::string_view, float*, float, float, float, bool) const override;
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
        IMGUI_INLINE_GETTER_DECL(LabelAndVec4)
        IMGUI_INLINE_GETTER_DECL(DragAndDropSource)
        IMGUI_INLINE_GETTER_DECL(DragAndDropTarget)
        IMGUI_INLINE_GETTER_DECL(LabelAndUInt16)
        IMGUI_INLINE_GETTER_DECL(ComboboxUInt8)
        IMGUI_INLINE_GETTER_DECL(Text)
        IMGUI_INLINE_GETTER_DECL(Separator)
        IMGUI_INLINE_GETTER_DECL(Table)
        IMGUI_INLINE_GETTER_DECL(TableRow)
        IMGUI_INLINE_GETTER_DECL(TableColumn)
        IMGUI_INLINE_GETTER_DECL(LabelAndVec2)

        void               NewFrame() override;
    };
}

namespace Engine::Managers
{
    ECLASS()
    class ENGINE_IMGUIMANAGER_API ImGuiManager final : public Abstracts::Singleton<ImGuiManager>
    {
        GENERATE_BODY
    public:
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
