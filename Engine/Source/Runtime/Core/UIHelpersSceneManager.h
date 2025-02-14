#pragma once
#if WITH_EDITOR
#include <functional>
#include "UIInterface.h"
#include "SceneManager/Public/SceneManager.h"
#include "Scene/Public/Scene.h"
#include "Layer/Public/Layer.h"

namespace Engine::UIHelpers
{
    using ObjectUIPredicateSignature = std::function<bool(const Strong<Abstracts::ObjectBase>&)>;
    using ObjectUITypePredicateSignature = std::function<bool(const HashType)>;

    // todo: gc, and multiple call safe dialog (a mesh <- a shape || b shape)
    template <typename T>
    static bool SingleObjectSelectionDialog(
        const Strong<Abstracts::Entity>& ptr,
        Weak<Abstracts::ObjectBase>& object_selected,
        const ObjectUIPredicateSignature& predicate = {},
        const ObjectUITypePredicateSignature& type_predicate = {})
    {
        using SelectionMap = std::unordered_map<Weak<Abstracts::ObjectBase>, bool>;
        using TypeSelectionMap = std::unordered_map<Weak<Abstracts::Entity>, SelectionMap>;

        bool window = true;
        static TypeSelectionMap selection{};

        UIInterface& ui = UIInterfaceAccessor::GetInterface();
        if (UIContext context = UIInterface::NewContext(ui.NewDialog({ ptr.get(), "Select Object...", window })))
        {
            context += ui.NewListBox({ "Object List", 0, 0 });

            for (const Strong<Scene>& scene : Managers::SceneManager::GetInstance().GetScenes())
            {
                context += ui.NewTreeNode({ scene->GetName() });

                for (const Strong<Layer>& layer : *scene)
                {
                    context += ui.NewTreeNode({ layer->GetName()});

                    for (const Weak<Abstracts::ObjectBase> object : layer->GetGameObjects())
                    {
                        if (const Strong<Abstracts::ObjectBase>& locked = object.lock())
                        {
                            if (predicate && !predicate(locked))
                            {
                                continue;
                            }

                            (context |= ui.NewSelectable({ locked->GetName(), selection[ptr][locked] })).SetFunction([&window]()
                                {
                                    window = false;
                                });
                        }
                    }
                    --context;
                }

                --context;
            }

            --context;
        }

        if (!window)
        {
            for (const auto& [key, flag] : selection[ptr])
            {
                if (const Strong<Abstracts::ObjectBase>& object = key.lock();
                    flag && object)
                {
                    object_selected = object;
                    break;
                }
            }

            selection.erase(ptr);
        }

        return !window;
    }

    template <typename T>
    static bool MultipleObjectSelectionDialog(
        const Strong<Abstracts::Entity>& ptr,
        std::vector<Weak<Abstracts::ObjectBase>>& object_selected,
        const ObjectUIPredicateSignature& predicate = {},
        const ObjectUITypePredicateSignature& type_predicate = {})
    {
        using SelectionMap = std::unordered_map<Weak<Abstracts::ObjectBase>, bool>;
        using TypeSelectionMap = std::unordered_map<Weak<Abstracts::Entity>, SelectionMap>;

        bool                                                       window = true;
        static TypeSelectionMap selection{};

        UIInterface& ui = UIInterfaceAccessor::GetInterface();
        if (UIContext context = UIInterface::NewContext(ui.NewDialog({ ptr.get(), "Select Objects...", window})))
        {
            context += ui.NewListBox({ "Object List", 0, 0 });

            for (const Strong<Scene>& scene : Managers::SceneManager::GetInstance().GetScenes())
            {
                context += ui.NewTreeNode({ scene->GetName() });

                for (const Strong<Layer>& layer : *scene)
                {
                    context += ui.NewTreeNode({ layer->GetName() });

                    for (const Weak<Abstracts::ObjectBase> object : layer->GetGameObjects())
                    {
                        if (const Strong<Abstracts::ObjectBase>& locked = object.lock())
                        {
                            if (predicate && !predicate(locked))
                            {
                                continue;
                            }

                            context |= ui.NewSelectable({ locked->GetName(), selection[ptr][locked] });
                        }
                    }
                    --context;
                }

                --context;
            }

            --context;

            (context |= ui.NewButton({ "Select" })).SetFunction([&window]()
                {
                    window = false;
                });
        }

        if (!window)
        {
            object_selected.clear();
            object_selected.reserve(selection[ptr].size());

            for (const auto& [key, flag] : selection[ptr])
            {
                if (const Strong<Abstracts::ObjectBase>& resource = key.lock();
                    flag && resource)
                {
                    object_selected.push_back(resource);
                }
            }

            selection.erase(ptr);
        }

        return !window;
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