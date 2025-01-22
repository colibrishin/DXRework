#include "../Public/ReflectionEvaluator.h"

#include "Source/Runtime/Managers/D3D12Wrapper/Public/D3Device.hpp"
#include "Source/Runtime/CommandPair/Public/CommandPair.h"
#include "Source/Runtime/DescriptorHeap/Public/Descriptors.h"

namespace Engine::Managers
{
	void ReflectionEvaluator::PreUpdate(const float& dt) {}

	void ReflectionEvaluator::Update(const float& dt) {}

	void ReflectionEvaluator::FixedUpdate(const float& dt) {}

	void ReflectionEvaluator::PreRender(const float& dt) {}

	void ReflectionEvaluator::Render(const float& dt) {}

	void ReflectionEvaluator::PostRender(const float& dt) {}

	void ReflectionEvaluator::PostUpdate(const float& dt) {}

	void ReflectionEvaluator::Initialize()
	{
		m_copy_.SetName("ReflectionEvaluator");
		m_copy_.Initialize();
		m_copy_.Load();
	}

	void ReflectionEvaluator::RenderFinished(const Weak<CommandPair>& w_cmd) const
	{
		D3Device::GetInstance().CopyBackBuffer(w_cmd, m_copy_.GetRawResoruce());
	}

	void ReflectionEvaluator::BindReflectionMap(const Weak<CommandPair>& w_cmd, const DescriptorPtr& heap) const
	{
		m_copy_.Bind(w_cmd, heap, BIND_TYPE_SRV, RESERVED_TEX_RENDERED, 0);
	}

	void ReflectionEvaluator::UnbindReflectionMap(const Weak<CommandPair>& w_cmd) const
	{
		m_copy_.Unbind(w_cmd, BIND_TYPE_SRV);
	}
}
