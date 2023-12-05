#pragma once

namespace Engine::Abstract
{
	// Empty interface for distinguishing state controllers
	class IStateController : public Component
	{
	public:
		explicit IStateController(const Component& component)
			: Component(component)
		{
		}

		IStateController(eComponentPriority priority, const WeakObject& owner)
			: Component(priority, owner)
		{
		}
	};
}