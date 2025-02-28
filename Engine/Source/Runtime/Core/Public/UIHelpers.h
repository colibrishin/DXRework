#pragma once
#if WITH_EDITOR
#include <functional>
#include <unordered_map>
#include <string>
#include <mutex>


#include "IUIAPI.h"

namespace Engine::UIHelpers
{
    using UICallbackSignature = std::function<void( UIContext *const )>;
    using NameAndPathConfirmCallbackSignature = std::function<void( const std::string &, const std::string & )>;
    using UICleanupCallbackSignature = std::function<void()>;
    using ManagedBooleanSignature = std::function<void( bool & )>;

    template <typename Key>
    using ManagedBoolAndFuncMap = std::unordered_map<Key, std::pair<bool, ManagedBooleanSignature> >;

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
        IUIAPI& ui = g_ui_accessor.GetInterface();

        static bool        pressed = false;
        static std::string name{};
        static std::string path{};

        if ( UIContext context = IUIAPI::NewContext( ui.NewDialog( pointer, "NewPathDialog", { title, flag } ) ) )
        {
            if constexpr ( UseName )
            {
                context |= ui.NewLabelAndText( pointer, "NamePathDialogName", { "Name", name, true } );
            }

            if constexpr ( UsePath )
            {
                context |= ui.NewLabelAndText( pointer, "NamePathDialogPath", { "Path", path, true } );
            }

            if ( ui_callback )
            {
                ui_callback( &context );
            }

            ( context |= ui.NewButton( pointer, "NewPathDialogConfirmButton", { confirm_button_label } ) ).SetFunction(
                    [&]()
                    {
                        pressed = true;
                        flag    = false;
                    }
                    );

            ( context |= ui.NewButton( pointer, "NewPathDialogCancelButton", { "Cancel" } ) ).SetFunction( [ & ]()
            {
                flag = false;
            } );
        }

        if ( pressed )
        {
            if ( confirm_callback )
            {
                confirm_callback( name, path );
            }
            if ( cleanup_callback )
            {
                cleanup_callback();
            }
            name    = {};
            path    = {};
            flag    = false;
            pressed = false;
            return false;
        }

        if ( !flag )
        {
            if ( cleanup_callback )
            {
                cleanup_callback();
            }
            name = {};
            path = {};
            return false;
        }

        return true;
    }

    template <typename T, typename U>
    bool OpenLoadDialog(
            const U &                                  caller,
            bool &                                     flag,
            const UICallbackSignature &                ui_callback,
            const NameAndPathConfirmCallbackSignature &load_callback,
            const UICleanupCallbackSignature &         cleanup_callback )
    {
        static std::string    title = "Load ";
        static std::once_flag initialized;
        std::call_once( initialized,
                        []()
                        {
                            title += T::StaticTypeName();
                        } );

        return NamePathDialogTemplate<false, true>( &caller,
                                                    flag,
                                                    title,
                                                    "Load",
                                                    ui_callback,
                                                    load_callback,
                                                    cleanup_callback );
    }

    template <typename T, typename U>
    bool OpenNewDialog(
        const U&                                   caller,
        bool&                                      flag,
        const UICallbackSignature&                 ui_callback,
        const NameAndPathConfirmCallbackSignature& load_callback,
        const UICleanupCallbackSignature&          cleanup_callback )
    {
        static std::string    title = "New ";
        static std::once_flag initialized;
        std::call_once(initialized, []() { title += T::StaticTypeName(); });

        return NamePathDialogTemplate<true, true>(&caller, flag, title, "Confirm", ui_callback, load_callback, cleanup_callback);
    }

    template <typename T, typename U>
    bool OpenLoadDialog(
        bool&                                      flag,
        const UICallbackSignature&                 ui_callback,
        const NameAndPathConfirmCallbackSignature& load_callback,
        const UICleanupCallbackSignature&          cleanup_callback
    )
    {
        return OpenLoadDialog<T, U>(U::GetInstance(), flag, ui_callback, load_callback, cleanup_callback);
    }

    template <typename T, typename U>
    bool OpenNewDialog(
        bool&                                      flag,
        const UICallbackSignature&                 ui_callback,
        const NameAndPathConfirmCallbackSignature& load_callback,
        const UICleanupCallbackSignature&          cleanup_callback
    )
    {
        return OpenNewDialog<T, U>(U::GetInstance(), flag, ui_callback, load_callback, cleanup_callback);
    }

    template <typename T> requires std::is_base_of_v<Abstracts::Entity, T>
    using SelectionMap = std::unordered_map<Weak<T>, bool>;

    template <typename T> requires std::is_base_of_v<Abstracts::Entity, T>
    using TypeSelectionMap = std::unordered_map<Weak<Abstracts::Entity>, SelectionMap<T>>;

    template <typename T, typename U, typename ContIterator, typename Cont>
    static bool SingleSelectionDialog(
            const Cont &                                    container,
            const Strong<Abstracts::Entity> &               ptr,
            Weak<U> &                                       selected,
            const std::function<bool( const Strong<U> & )> &predicate      = {},
            const std::function<bool( const HashType )> &   type_predicate = {} )
    {
        bool                       window = true;
        static TypeSelectionMap<U> selection{};

        IUIAPI &ui = g_ui_accessor.GetInterface();

        static ContIterator      iterator{};
        static const std::string dialog_id    = std::format( "Single{}SelectionDialog", U::StaticTypeName() );
        static const std::string dialog_title = std::format( "Select {}...", U::StaticTypeName() );
        static const std::string dialog_listbox_name = std::format( "Single{}SelectionDialogListBox", U::StaticTypeName() );
        static const std::string dialog_confirm = std::format( "Single{}SelectionDialogListBoxConfirmButton", U::StaticTypeName() );

        if ( UIContext context = IUIAPI::NewContext( ui.NewDialog(
                ptr.get(),
                dialog_id,
                { dialog_title, window } ) ) )
        {
            context += ui.NewListBox( ptr.get(), dialog_listbox_name, { "List", 0, 0 } );
            iterator( container, &context, ptr, predicate, type_predicate, selection, window );
            --context;

            (context |= ui.NewButton( ptr.get(), dialog_confirm, { "Confirm" } )).SetFunction( [&window]()
            {
                window = false;
            } );
        }

        if ( !window )
        {
            for ( const auto &[ key, flag ] : selection[ ptr ] )
            {
                if ( const Strong<U> &object = key.lock();
                    flag && object )
                {
                    selected = object;
                    break;
                }
            }

            selection.erase( ptr );
        }

        return !window;
    }

    template <typename T, typename U, typename ContIterator, typename Cont>
    static bool MultipleSelectionDialog(
            const Cont &                                    container,
            const Strong<Abstracts::Entity> &               ptr,
            std::vector<Weak<U> > &                         selected,
            const std::function<bool( const Strong<U> & )> &predicate      = {},
            const std::function<bool( const HashType )> &   type_predicate = {} )
    {
        bool                       window = true;
        static TypeSelectionMap<U> selection{};

        IUIAPI &ui = g_ui_accessor.GetInterface();

        static ContIterator      iterator{};
        static const std::string dialog_id    = std::format( "Multiple{}SelectionDialog", U::StaticTypeName() );
        static const std::string dialog_title = std::format( "Select {}...", U::StaticTypeName() );
        static const std::string dialog_listbox_name = std::format( "Multiple{}SelectionDialogListBox", U::StaticTypeName() );
        static const std::string dialog_confirm = std::format( "Multiple{}SelectionDialogListBoxConfirmButton", U::StaticTypeName() );


        if ( UIContext context = IUIAPI::NewContext( ui.NewDialog(
                ptr.get(),
                dialog_id,
                { dialog_title, window } ) ) )
        {
            context += ui.NewListBox( ptr.get(), dialog_listbox_name, { "List", 0, 0 } );
            iterator( container, &context, ptr, predicate, type_predicate, selection, window );
            --context;
            ( context |= ui.NewButton( ptr.get(), dialog_confirm, { "Confirm" } ) ).SetFunction( [&window]()
            {
                window = false;
            } );
        }

        if ( !window )
        {
            selected.clear();
            selected.reserve( selection[ ptr ].size() );

            for ( const auto &[ key, flag ] : selection[ ptr ] )
            {
                if ( const Strong<U> &resource = key.lock();
                    flag && resource )
                {
                    selected.push_back( resource );
                }
            }

            selection.erase( ptr );
        }

        return !window;
    }
}
#endif