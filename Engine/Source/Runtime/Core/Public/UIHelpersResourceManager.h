#pragma once
#if WITH_EDITOR
#include <functional>
#include "UIHelpers.h"
#include "IUIAPI.h"
#include "ResourceManager.h"

namespace Engine::UIHelpers
{
    using ResourceUIPredicateSignature = std::function<bool(const Strong<Abstracts::Resource>&)>;
    using ResourceUITypePredicateSignature = std::function<bool(const ResourceType)>;

    template <bool Multiple>
    struct UIResourceIterator
    {
        void operator()( const auto &                            container,
                         UIContext *const                        ui_context,
                         const Strong<Abstracts::Entity> &       ptr,
                         const ResourceUIPredicateSignature &    predicate,
                         const ResourceUITypePredicateSignature &type_predicate,
                         TypeSelectionMap<Abstracts::Resource> & selection_map,
                         bool &                                  window ) const
        {
            IUIAPI &ui = g_ui_accessor.GetInterface();

            for ( auto it = container.begin(); it != container.end(); ++it )
            {
                const auto & [ type, resources ] = *it;
                const size_t idx                 = std::distance( container.begin(), it );

                if ( resources.empty() )
                {
                    continue;
                }

                if ( type_predicate && !type_predicate( type ) )
                {
                    continue;
                }

                const std::string_view type_name = ( *resources.begin() )->GetPrettyTypeName();
                *ui_context += ui.NewTreeNode( ptr.get(),
                                               std::format( "ResourceSelectionTree{}", idx ),
                                               { type_name } );

                for ( auto rit = resources.begin(); rit != resources.end(); ++rit )
                {
                    const Strong<Abstracts::Resource> &resource = *rit;
                    const size_t                       ridx     = std::distance( resources.begin(), rit );

                    if ( resource == ptr )
                    {
                        continue;
                    }

                    if ( predicate && !predicate( resource ) )
                    {
                        continue;
                    }

                    ( *ui_context |= ui.NewSelectable( ptr.get(),
                                                       std::format( "ResourceSelectionItem{}", ridx ),
                                                       { resource->GetName(), selection_map[ ptr ][ resource ] } ) ).
                            SetFunction( [&window]()
                            {
                                if constexpr (!Multiple)
                                {
                                    window = false;         
                                }
                            } );
                }

                --*ui_context;
            }
        }
    };

    template <typename T>
    static bool SingleResourceSelectionDialog(
            const Strong<Abstracts::Entity> &       ptr,
            Weak<Abstracts::Resource> &             resources_to_load,
            const ResourceUIPredicateSignature &    predicate      = {},
            const ResourceUITypePredicateSignature &type_predicate = {} )
    {
        return SingleSelectionDialog<T, Abstracts::Resource, UIResourceIterator<false>>(
                Managers::ResourceManager::GetInstance().GetResources(),
                ptr,
                resources_to_load,
                predicate,
                type_predicate );
    }

    template <typename T>
    static bool MultipleResourceSelectionDialog(
            const Strong<Abstracts::Entity> &        ptr,
            std::vector<Weak<Abstracts::Resource> > &resources_to_load,
            const ResourceUIPredicateSignature &     predicate      = {},
            const ResourceUITypePredicateSignature & type_predicate = {} )
    {

        return MultipleSelectionDialog<T, Abstracts::Resource, UIResourceIterator<true>>(
                Managers::ResourceManager::GetInstance().GetResources(),
                ptr,
                resources_to_load,
                predicate,
                type_predicate );
    }

    template <typename T, typename... Excluded>
    static bool MultipleResourceSelectionDialogExclusion(
        const Strong<Abstracts::Entity>& ptr,
        std::vector<Weak<Abstracts::Resource>>& resources_to_load)
    {
        const auto& type_pred = [](const ResourceType type)
            {
                bool retval[] = { type->IsDerivedOf(Excluded::StaticTypeHash())...};
                return !std::any_of(std::begin(retval), std::end(retval), [](const bool b) {return b == true; });
            };

        return MultipleResourceSelectionDialog<T>(ptr, resources_to_load, {}, type_pred);
    }

    template <typename T, typename... Included>
    static bool MultipleResourceSelectionDialogInclusion(
        const Strong<Abstracts::Entity>& ptr,
        std::vector<Weak<Abstracts::Resource>>& resources_to_load)
    {
        const auto& type_pred = [](const ResourceType type)
            {
                bool retval[] = { type->IsDerivedOf(Included::StaticTypeHash())... };
                return std::any_of(std::begin(retval), std::end(retval), [](const bool b) {return b == true; });
            };

        return MultipleResourceSelectionDialog<T>(ptr, resources_to_load, {}, type_pred);
    }

    template <typename T, typename... Excluded>
    static bool SingleResourceSelectionDialogExclusion(
        const Strong<Abstracts::Entity>& ptr,
        Weak<Abstracts::Resource>& resources_to_load)
    {
        const auto& type_pred = [](const ResourceType type)
            {
                bool retval[] = { type->IsDerivedOf(Excluded::StaticTypeHash())... };
                return !std::any_of(std::begin(retval), std::end(retval), [](const bool b) {return b == true; });
            };

        return SingleResourceSelectionDialog<T>(ptr, resources_to_load, {}, type_pred);
    }

    template <typename T, typename... Included>
    static bool SingleResourceSelectionDialogInclusion(
        const Strong<Abstracts::Entity>& ptr,
        Weak<Abstracts::Resource>& resources_to_load)
    {
        const auto& type_pred = [](const ResourceType type)
            {
                bool retval[] = { type->IsDerivedOf(Included::StaticTypeHash())... };
                return std::any_of(std::begin(retval), std::end(retval), [](const bool b) {return b == true; });
            };

        return SingleResourceSelectionDialog<T>(ptr, resources_to_load, {}, type_pred);
    }
}
#endif