#pragma once
#include "Source/Runtime/Core/Singleton/Public/Singleton.hpp"
#include <directx/d3d12.h>
#include "Source/Runtime/Resources/Texture2D/Public/Texture2D.h"

namespace Engine 
{
	struct CommandPair;
}

namespace Engine::Managers
{
	class REFLECTIONEVALUATOR_API ReflectionEvaluator : public Abstracts::Singleton<ReflectionEvaluator>
	{
	public:
		ReflectionEvaluator(SINGLETON_LOCK_TOKEN)
			: Singleton(),
			  m_copy_
			  (
			   "", {
				   .Alignment = 0,
				   .Width = CFG_WIDTH,
				   .Height = CFG_HEIGHT,
				   .DepthOrArraySize = 1,
				   .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				   .Flags = D3D12_RESOURCE_FLAG_NONE,
				   .MipsLevel = 1,
				   .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
				   .SampleDesc = {.Count = 1, .Quality = 0}
			   }
			  ) {}

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;
		void PostUpdate(const float& dt) override;
		void Initialize() override;

		void RenderFinished(const Weak<CommandPair>& w_cmd) const;
		void BindReflectionMap(const Weak<CommandPair>& w_cmd, const DescriptorPtr& heap) const;
		void UnbindReflectionMap(const Weak<CommandPair>& w_cmd) const;

	private:
		friend struct SingletonDeleter;
		~ReflectionEvaluator() override = default;

		Resources::Texture2D m_copy_;
	};
}