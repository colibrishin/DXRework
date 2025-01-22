#pragma once
#include <functional>

#include "UIInterface.h"

namespace Engine::UIHelpers
{
    using UICallbackSignature                 = std::function<void(UIContext* const)>;
    using NameAndPathConfirmCallbackSignature = std::function<void(const std::string&, const std::string&)>;
    using UICleanupCallbackSignature          = std::function<void()>;
    using ManagedBooleanSignature = std::function<void(bool&)>;
    template <typename Key>
    using ManagedBoolAndFuncMap = std::unordered_map<Key, std::pair<bool, ManagedBooleanSignature>>;
    
    template <bool UseName, bool UsePath>
    bool NamePathDialogTemplate(
        const void*                                pointer,
        bool&                                      flag,
        const std::string_view                     title,
        const std::string_view                     confirm_button_label,
        const UICallbackSignature&                 ui_callback,
        const NameAndPathConfirmCallbackSignature& confirm_callback,
        const UICleanupCallbackSignature&          cleanup_callback
    )
    {
        UIInterface& ui = UIInterfaceAccessor::GetInterface();

        static bool        pressed = false;
        static std::string name{};
        static std::string path{};

        if (UIContext context = UIInterface::NewContext(ui.NewDialog({pointer, title, flag})))
        {
            if constexpr (UseName) { context |= ui.NewLabelAndText({"Name", name, true}); }

            if constexpr (UsePath) { context |= ui.NewLabelAndText({"Path", path, true}); }

            if (ui_callback) { ui_callback(&context); }

            (context |= ui.NewButton({confirm_button_label})).SetFunction
                (
                 [&]()
                 {
                     pressed = true;
                     flag    = false;
                 }
                );

            (context |= ui.NewButton({"Cancel"})).SetFunction([&]() { flag = false; });
        }

        if (pressed)
        {
            if (confirm_callback) { confirm_callback(name, path); }
            if (cleanup_callback) { cleanup_callback(); }
            name    = {};
            path    = {};
            flag    = false;
            pressed = false;
            return false;
        }

        if (!flag)
        {
            if (cleanup_callback) { cleanup_callback(); }
            name = {};
            path = {};
            return false;
        }

        return true;
    }

    template <typename T, typename U>
    bool OpenLoadDialog(
        const U&                                   caller,
        bool&                                      flag,
        const UICallbackSignature&                 ui_callback,
        const NameAndPathConfirmCallbackSignature& load_callback,
        const UICleanupCallbackSignature&          cleanup_callback
    )
    {
        static std::string    title = "Load ";
        static std::once_flag initialized;
        std::call_once(initialized, []() { title += T::StaticTypeName(); });

        return NamePathDialogTemplate<false, true>(&caller, flag, title, "Load", ui_callback, load_callback, cleanup_callback);
    }

    template <typename T, typename U>
    bool OpenNewDialog(
        const U&                                   caller,
        bool&                                      flag,
        const UICallbackSignature&                 ui_callback,
        const NameAndPathConfirmCallbackSignature& load_callback,
        const UICleanupCallbackSignature&          cleanup_callback
    )
    {
        static std::string    title = "New ";
        static std::once_flag initialized;
        std::call_once(initialized, []() { title += T::StaticTypeName(); });

        return NamePathDialogTemplate<true, true>(&caller, flag, title, "Confirm", ui_callback, load_callback, cleanup_callback);
    }

    template <typename T, typename U> requires (std::is_base_of_v<Abstracts::SingletonBase, U>)
    bool OpenLoadDialog(
        bool&                                      flag,
        const UICallbackSignature&                 ui_callback,
        const NameAndPathConfirmCallbackSignature& load_callback,
        const UICleanupCallbackSignature&          cleanup_callback
    )
    {
        return OpenLoadDialog<T, U>(U::GetInstance(), flag, ui_callback, load_callback, cleanup_callback);
    }

    template <typename T, typename U> requires (std::is_base_of_v<Abstracts::SingletonBase, U>)
    bool OpenNewDialog(
        bool&                                      flag,
        const UICallbackSignature&                 ui_callback,
        const NameAndPathConfirmCallbackSignature& load_callback,
        const UICleanupCallbackSignature&          cleanup_callback
    )
    {
        return OpenNewDialog<T, U>(U::GetInstance(), flag, ui_callback, load_callback, cleanup_callback);
    }
}
