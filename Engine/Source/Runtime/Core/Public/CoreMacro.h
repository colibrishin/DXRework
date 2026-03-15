#pragma once

#include <filesystem>
#include <string_view>
#include <tuple>
#include <type_traits>

// True when Args is empty, or when the first of Args is not convertible to std::filesystem::path (avoids tuple_element_t<0, tuple<>).
namespace Engine::Detail
{
	template <typename... Args>
	struct FirstArgNotPathLike : std::true_type
	{
	};
	template <typename First, typename... Rest>
	struct FirstArgNotPathLike<First, Rest...>
		: std::integral_constant<bool, !std::is_constructible_v<std::filesystem::path, First>>
	{
	};
}

// Static resource getter which infers self as type. Matches header-parser generated getters (name, metadata path).
// Trailing return type avoids '>' parse ambiguity when macro is expanded inside class body.
#define RESOURCE_SELF_INFER_GETTER(TYPE) \
	template <typename Void = void, typename Name> requires (std::is_constructible_v<std::string_view, Name>) \
	static auto Get(const Name& name) -> Engine::Weak<TYPE> { return Engine::Managers::ResourceManager::GetInstance().GetResource<TYPE>(name); } \
	template <typename Void = void, typename MetaPath> requires (std::is_constructible_v<std::filesystem::path, MetaPath>) \
	static auto GetByMetadataPath(const MetaPath& meta_path) -> Engine::Weak<TYPE> { return Engine::Managers::ResourceManager::GetInstance().GetResourceByMetadataPath<TYPE>(meta_path); }

// Static resource create function (path + args overload: RawPath converted to path).
// Only requires RawPath convertible to path (no TYPE constraint: is_constructible considers access, so protected path ctor would fail).
#define RESOURCE_SELF_INFER_CREATE_PATH_OVERLOAD(TYPE)\
template <typename Name, typename RawPath, typename... Args>\
	requires (std::is_constructible_v<std::string_view, Name>,\
	          std::is_constructible_v<std::filesystem::path, RawPath>)\
static auto Create(const Name& name, const RawPath& raw_path, Args&&... args) -> Engine::Strong<TYPE>\
{\
	const std::string_view name_view(name);\
	if (!name_view.empty() && Engine::Managers::ResourceManager::GetInstance().GetResource<TYPE>(name_view).lock()) { return {}; }\
	const std::filesystem::path path_view(raw_path);\
	const auto obj = make_managed_shared<TYPE>(path_view, std::forward<Args>(args)...);\
	Engine::Managers::ResourceManager::GetInstance().AddResource(name_view, obj);\
	if (!obj->IsLoaded()) { obj->Load(); }\
	return obj;\
}

// Static resource create function (args-only overload): selected when second argument is not path-like (so path overload is not viable).
#define RESOURCE_SELF_INFER_CREATE_ARGS_OVERLOAD(TYPE)\
template <typename Name, typename... Args>\
	requires (std::is_constructible_v<std::string_view, Name>,\
	          Engine::Detail::FirstArgNotPathLike<Args...>::value)\
static auto Create(const Name& name, Args&&... args) -> Engine::Strong<TYPE>\
{\
	const std::string_view name_view(name);\
	if (!name_view.empty() && Engine::Managers::ResourceManager::GetInstance().GetResource<TYPE>(name_view).lock()) { return {}; }\
	const auto obj = make_managed_shared<TYPE>(std::forward<Args>(args)...);\
	Engine::Managers::ResourceManager::GetInstance().AddResource(name_view, obj);\
	if (!obj->IsLoaded()) { obj->Load(); }\
	return obj;\
}

// Static resource create: both path and args-only overloads (same requires logic as header-parser generated code).
#define RESOURCE_SELF_INFER_CREATE(TYPE)\
	RESOURCE_SELF_INFER_CREATE_PATH_OVERLOAD(TYPE)\
	RESOURCE_SELF_INFER_CREATE_ARGS_OVERLOAD(TYPE)

// Cloning resource declaration macro
#define RES_CLONE_DECL Engine::Strong<Engine::Abstracts::Resource> cloneImpl() const override;
// Cloning resource implementation macro
#define RES_CLONE_IMPL(CLASS) Engine::Strong<Engine::Abstracts::Resource> CLASS::cloneImpl() const { return make_managed_shared<CLASS>(*this); }
