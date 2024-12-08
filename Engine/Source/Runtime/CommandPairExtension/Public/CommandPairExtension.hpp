#pragma once
#include <functional>
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

namespace Engine 
{
	struct CommandPair;
	struct DescriptorPtrImpl;

	struct COMMANDPAIREXTENSION_API CommandPairExtension 
	{
	public:
		using CommandDescriptorLambda = std::function<void(const Weak<CommandPair>&, const Weak<DescriptorPtrImpl>&) > ;
	};
}