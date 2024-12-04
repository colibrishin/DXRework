#pragma once
#include "Source/Runtime/Abstracts/CoreSingleton/Public/Singleton.hpp"

#include "Source/Runtime/StructuredBufferDX12/Public/StructuredBufferDX12.hpp"
#include "Source/Runtime/DescriptorHeap/Public/Descriptors.h"
#include "Source/Runtime/StructuredBufferDX12/Public/StructuredBufferMemoryPoolDX12.hpp"
#include "Source/Runtime/Allocator/Public/Allocator.h"
#include "Source/Runtime/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/Abstracts/CoreObjectBase/Public/ObjectBase.hpp"
#include "Source/Runtime/Resources/Material/Public/Material.h"
#include "Source/Runtime/Components/RenderComponent/Public/egRenderComponent.h"
#include "Source/Runtime/Managers/CommonRenderer/Public/CommonRenderer.h"

namespace Engine::Managers
{
	using namespace Engine::Graphics;
	using namespace Engine::Rendering;

	class Renderer : public Abstracts::Singleton<Renderer>
	{
	public:
		explicit Renderer(SINGLETON_LOCK_TOKEN)
			: Singleton(),
			  m_b_ready_(false) {}

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;
		void PostUpdate(const float& dt) override;
		void Initialize() override;

		void AppendAdditionalStructuredBuffer(StructuredBufferBase* sb_ptr);

		bool Ready() const;
		void RenderPass(
			float                                        dt,
			eShaderDomain                                domain,
			bool                                         shader_bypass,
			const Weak<CommandPair>&                     w_cmd,
			DescriptorContainer&                         descriptor_heap_container,
			StructuredBufferMemoryPool<SBs::InstanceSB>& instance_buffer_memory_pool,
			const ObjectPredication&                     predicate,
			const CommandDescriptorLambda&               initial_setup,
			const CommandDescriptorLambda&               post_setup, 
			const std::vector<StructuredBufferBase*>&    additional_structured_buffers
		);

		UINT64 GetInstanceCount() const;

	private:
		friend struct SingletonDeleter;
		friend class RayTracer;
		~Renderer() override = default;

		void renderPassImpl(
			const float                         dt,
			eShaderDomain                       domain,
			bool                                shader_bypass,
			StructuredBuffer<SBs::InstanceSB> & instance_buffer,
			const Strong<Resources::Material> &              material,
			const Weak<CommandPair> &           w_cmd,
			const DescriptorPtr &               heap, const aligned_vector<const SBs::InstanceSB*> & structured_buffers
		);

		bool m_b_ready_;

		std::vector<StructuredBufferBase*>          m_additional_structured_buffers_;
		StructuredBufferMemoryPool<SBs::InstanceSB> m_tmp_instance_buffers_;
		DescriptorContainer      m_tmp_descriptor_heaps_;

		std::atomic<UINT64> m_instance_count_;
		std::atomic<UINT64> m_current_instance_;

		RenderMap m_render_candidates_[SHADER_DOMAIN_MAX];
	};
}
