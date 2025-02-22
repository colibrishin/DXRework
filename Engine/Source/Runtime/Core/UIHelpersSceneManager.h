#pragma once
#if WITH_EDITOR
#include <functional>
#include "UIInterface.h"
#include "UIHelpers.h"
#include "SceneManager/Public/SceneManager.h"

#include "ObjectBase/Public/ObjectBase.h"

namespace Engine::UIHelpers
{
    using ObjectUIPredicateSignature = std::function<bool(const Strong<Abstracts::ObjectBase>&)>;
    using ObjectUITypePredicateSignature = std::function<bool(const HashType)>;

    template <bool Multiple>
    struct UISceneIterator
    {
        void operator()( const auto &                             container,
                         UIContext *const                         ui_context,
                         const Strong<Abstracts::Entity> &        ptr,
                         const ObjectUIPredicateSignature &       predicate,
                         const ObjectUITypePredicateSignature &   type_predicate,
                         TypeSelectionMap<Abstracts::ObjectBase> &selection_map,
                         bool &                                   window ) const
        {
            UIInterface &ui = UIInterfaceAccessor::GetInterface();

            for ( auto it = container.begin(); it != container.end(); ++it )
            {
                const Strong<Scene> &scene = *it;
                const ptrdiff_t      idx   = std::distance( container.begin(), it );
                *ui_context += ui.NewTreeNode( ptr.get(), std::format( "Scene{}TreeNode", idx ), { scene->GetName() } );

                for ( auto lit = scene->begin(); lit != scene->end(); ++it )
                {
                    const Strong<Layer> &layer = *lit;
                    const ptrdiff_t      lidx  = std::distance( scene->begin(), lit );
                    *ui_context += ui.NewTreeNode( this, std::format( "Layer{}", lidx ), { layer->GetName() } );

                    const auto &layer_objects = layer->GetGameObjects();
                    for ( auto oit = layer_objects.begin(); oit != layer_objects.end(); ++oit )
                    {
                        const Weak<Abstracts::ObjectBase> object = *oit;
                        const size_t                      oidx   = std::distance( layer_objects.begin(), oit );

                        if ( const Strong<Abstracts::ObjectBase> &locked = object.lock() )
                        {
                            if ( predicate && !predicate( locked ) )
                            {
                                continue;
                            }

                            ( *ui_context |= ui.NewSelectable( ptr.get(),
                                                               std::format( "ObjectSelectable{}", oidx ),
                                                               { locked->m_ui_info_.label,
                                                                 selection_map[ ptr ][ locked ] } ) ).SetFunction(
                                    [&window]()
                                    {
                                        if constexpr (!Multiple)
                                        {
                                            window = false;   
                                        }
                                    } );
                        }
                    }
                    --*ui_context;
                }
                --*ui_context;
            }
        }
    };

    template <typename T>
    static bool SingleObjectSelectionDialog(
            const Strong<Abstracts::Entity> &     ptr,
            Weak<Abstracts::ObjectBase> &         object_selected,
            const ObjectUIPredicateSignature &    predicate      = {},
            const ObjectUITypePredicateSignature &type_predicate = {} )
    {
        return SingleSelectionDialog<T, Abstracts::ObjectBase, UISceneIterator<false>>(
                Managers::SceneManager::GetInstance().GetScenes(),
                ptr,
                object_selected,
                predicate,
                type_predicate );
    }

    template <typename T>
    static bool MultipleObjectSelectionDialog(
        const Strong<Abstracts::Entity>& ptr,
        std::vector<Weak<Abstracts::ObjectBase>>& object_selected,
        const ObjectUIPredicateSignature& predicate = {},
        const ObjectUITypePredicateSignature& type_predicate = {})
    {
        return MultipleSelectionDialog<T, Abstracts::ObjectBase, UISceneIterator<true>>(
                Managers::SceneManager::GetInstance().GetScenes(),
                ptr,
                object_selected,
                predicate,
                type_predicate );
    }

    template <typename T, typename... Excluded>
    static bool MultipleObjectSelectionDialogExclusion(
        const Strong<Abstracts::Entity>& ptr,
        std::vector<Weak<Abstracts::ObjectBase>>& object_selected)
    {
        const auto& type_pred = [](const HashType type)
            {
                bool retval[] = { type->IsDerivedOf(Excluded::StaticTypeHash())...};
                return !std::any_of(std::begin(retval), std::end(retval), [](const bool b) {return b == true; });
            };

        return MultipleObjectSelectionDialog<T>(ptr, object_selected, {}, type_pred);
    }

    template <typename T, typename... Included>
    static bool MultipleObjectSelectionDialogInclusion(
        const Strong<Abstracts::Entity>& ptr,
        std::vector<Weak<Abstracts::ObjectBase>>& object_selected)
    {
        const auto& type_pred = [](const HashType type)
            {
                bool retval[] = { type->IsDerivedOf(Included::StaticTypeHash())... };
                return std::any_of(std::begin(retval), std::end(retval), [](const bool b) {return b == true; });
            };

        return MultipleResourceSelectionDialog<T>(ptr, object_selected, {}, type_pred);
    }

    template <typename T, typename... Excluded>
    static bool SingleObjectSelectionDialogExclusion(
        const Strong<Abstracts::Entity>& ptr,
        Weak<Abstracts::ObjectBase>& object_selected)
    {
        const auto& type_pred = [](const HashType type)
            {
                bool retval[] = { type->IsDerivedOf(Excluded::StaticTypeHash())... };
                return !std::any_of(std::begin(retval), std::end(retval), [](const bool b) {return b == true; });
            };

        return SingleObjectSelectionDialog<T>(ptr, object_selected, {}, type_pred);
    }

    template <typename T, typename... Included>
    static bool SingleObjectSelectionDialogInclusion(
        const Strong<Abstracts::Entity>& ptr,
        Weak<Abstracts::ObjectBase>& object_selected)
    {
        const auto& type_pred = [](const HashType type)
            {
                bool retval[] = { type->IsDerivedOf(Included::StaticTypeHash())... };
                return std::any_of(std::begin(retval), std::end(retval), [](const bool b) {return b == true; });
            };

        return SingleObjectSelectionDialog<T>(ptr, object_selected, {}, type_pred);
    }
}
#endif