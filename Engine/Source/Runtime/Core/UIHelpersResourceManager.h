#pragma once
#if WITH_EDITOR
#include <functional>
#include "UIInterface.h"
#include "ResourceManager/Public/ResourceManager.h"

namespace Engine::UIHelpers
{
    using ResourceUIPredicateSignature = std::function<bool(const Strong<Abstracts::Resource>&)>;
    using ResourceUITypePredicateSignature = std::function<bool(const ResourceType)>;

    // todo: gc, and multiple call safe dialog (a mesh <- a shape || b shape)
    template <typename T>
    static bool SingleResourceSelectionDialog(
        const Strong<Abstracts::Entity>& ptr,
        Weak<Abstracts::Resource>& resources_to_load,
        const ResourceUIPredicateSignature& predicate = {},
        const ResourceUITypePredicateSignature& type_predicate = {})
    {
        using SelectionMap = std::unordered_map<Weak<Abstracts::Resource>, bool>;
        using TypeSelectionMap = std::unordered_map<Weak<Abstracts::Entity>, SelectionMap>;

        bool window = true;
        static TypeSelectionMap selection{};

        UIInterface& ui = UIInterfaceAccessor::GetInterface();
        if (UIContext context = UIInterface::NewContext(ui.NewDialog({ ptr.get(), "Add Resources to...", window })))
        {
            context += ui.NewListBox({ "Resource List", 0, 0 });

            for (const auto& [type, resources] : Managers::ResourceManager::GetInstance().GetResources())
            {
                if (resources.empty())
                {
                    continue;
                }

                if (type_predicate && !type_predicate(type))
                {
                    continue;
                }

                const std::string_view type_name = (*resources.begin())->GetPrettyTypeName();

                context += ui.NewTreeNode({ type_name });

                for (const Strong<Abstracts::Resource>& resource : resources)
                {
                    if (resource == ptr)
                    {
                        continue;
                    }

                    if (predicate && !predicate(resource))
                    {
                        continue;
                    }

                    (context |= ui.NewSelectable({ resource->GetName(), selection[ptr][resource] })).SetFunction([&window]()
                        {
                            window = false;
                        });

                }

                --context;
            }

            --context;
        }

        if (!window)
        {
            for (const auto& [key, flag] : selection[ptr])
            {
                if (const Strong<Abstracts::Resource>& resource = key.lock();
                    flag && resource)
                {
                    if (!resource->IsLoaded())
                    {
                        resource->Load();
                    }

                    resources_to_load = resource;
                    break;
                }
            }

            selection.erase(ptr);
        }

        return !window;
    }

    template <typename T>
    static bool MultipleResourceSelectionDialog(
        const Strong<Abstracts::Entity>& ptr,
        std::vector<Weak<Abstracts::Resource>>& resources_to_load,
        const ResourceUIPredicateSignature& predicate = {},
        const ResourceUITypePredicateSignature& type_predicate = {})
    {
        using SelectionMap = std::unordered_map<Weak<Abstracts::Resource>, bool>;
        using TypeSelectionMap = std::unordered_map<Weak<Abstracts::Entity>, SelectionMap>;

        bool                                                       window = true;
        static TypeSelectionMap selection{};

        UIInterface& ui = UIInterfaceAccessor::GetInterface();
        if (UIContext context = UIInterface::NewContext(ui.NewDialog({ ptr.get(), "Add Resources to...", window})))
        {
            context += ui.NewListBox({ "Resource List", 0, 0 });

            for (const auto& [type, resources] : Managers::ResourceManager::GetInstance().GetResources())
            {
                if (resources.empty())
                {
                    continue;
                }

                if (type_predicate && !type_predicate(type)) 
                {
                    continue;
                }

                const std::string_view type_name = (*resources.begin())->GetPrettyTypeName();

                context += ui.NewTreeNode({ type_name });

                for (const Strong<Abstracts::Resource>& resource : resources)
                {
                    if (resource == ptr) 
                    {
                        continue;
                    }

                    if (predicate && !predicate(resource))
                    {
                        continue;
                    }

                    context |= ui.NewSelectable({ resource->GetName(), selection[ptr][resource] });
                }

                --context;
            }

            --context;

            (context |= ui.NewButton({ "Add Resources" })).SetFunction([&window]()
                {
                    window = false;
                });
        }

        if (!window)
        {
            resources_to_load.clear();
            resources_to_load.reserve(selection[ptr].size());

            for (const auto& [key, flag] : selection[ptr])
            {
                if (const Strong<Abstracts::Resource>& resource = key.lock();
                    flag && resource)
                {
                    if (!resource->IsLoaded())
                    {
                        resource->Load();
                    }

                    resources_to_load.push_back(resource);
                }
            }

            selection.erase(ptr);
        }

        return !window;
    }

    template <typename T, typename... Excluded>
    static bool MultipleResourceSelectionDialogExclusion(
        const Strong<Abstracts::Entity>& ptr,
        std::vector<Weak<Abstracts::Resource>>& resources_to_load)
    {
        const auto& type_pred = [](const ResourceType type)
            {
                bool retval[] = { type->IsBaseOf(Excluded::StaticTypeHash())...};
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
                bool retval[] = { type->IsBaseOf(Included::StaticTypeHash())... };
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
                bool retval[] = { type->IsBaseOf(Excluded::StaticTypeHash())... };
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
                bool retval[] = { type->IsBaseOf(Included::StaticTypeHash())... };
                return std::any_of(std::begin(retval), std::end(retval), [](const bool b) {return b == true; });
            };

        return SingleResourceSelectionDialog<T>(ptr, resources_to_load, {}, type_pred);
    }
}
#endif