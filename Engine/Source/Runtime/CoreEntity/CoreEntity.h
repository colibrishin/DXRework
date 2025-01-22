#pragma once
#include <memory>
#include <boost/smart_ptr.hpp>
#include <string>
#include <filesystem>
#include "Source/Runtime/Misc.h"

namespace Engine
{
	template <typename T>
	using Weak = boost::weak_ptr<T>;

	template <typename T>
	using Strong = boost::shared_ptr<T>;

	template <typename T>
	using Unique = std::unique_ptr<T>;
	
	using GenericString = std::string;
	using EntityName = GenericString;

	using TypeName = std::string_view;
	using MetadataPathStr = GenericString;
	using MetadataPath = std::filesystem::path;

	using IDType = uint32_t;
	using GlobalEntityID = IDType;

	namespace Abstracts
	{
		class Entity;
		class Renderable;
	} // namespace Abstracts

#if WITH_EDITOR
	struct UIContext;
#endif
}
