#pragma once
#include <functional>
#include "TypeLibrary.h"

namespace Engine 
{
	struct CommandPair;
	struct DescriptorPtrImpl;
	using CommandDescriptorLambda = std::function<void(const Weak<CommandPair>&, const Weak<DescriptorPtrImpl>&)>;
}
