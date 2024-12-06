#pragma once
#include <directxtk12/CommonStates.h>
#include <directxtk12/DescriptorHeap.h>
#include <directxtk12/ResourceUploadBatch.h>
#include <directxtk12/SpriteBatch.h>
#include <directxtk12/PrimitiveBatch.h>
#include <directxtk12/Effects.h>
#include <directxtk12/GeometricPrimitive.h>

#include "Source/Runtime/Core/Singleton/Public/Singleton.hpp"
#include <directxtk12/EffectPipelineStateDescription.h>

namespace Engine::Managers
{
	class ToolkitAPI final : public Abstracts::Singleton<ToolkitAPI>
	{
	public:
		explicit ToolkitAPI(SINGLETON_LOCK_TOKEN) {}

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostUpdate(const float& dt) override;

		void AppendSpriteBatch(const std::function<void()>& callback);
		void AppendPrimitiveBatch(const std::function<void()>& callback);

		DirectX::SpriteBatch*                         GetSpriteBatch() const;
		DirectX::CommonStates*                        GetCommonStates() const;
		DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* GetPrimitiveBatch() const;
		DirectX::DescriptorHeap*                      GetDescriptorHeap() const;

	private:
		friend struct SingletonDeleter;
		~ToolkitAPI() override;

		std::unique_ptr<DirectX::DescriptorHeap> m_descriptor_heap_ = nullptr;

		std::unique_ptr<DirectX::CommonStates>                        m_states_                = nullptr;
		std::unique_ptr<DirectX::GeometricPrimitive>                  m_geometric_primitive_   = nullptr;
		std::unique_ptr<DirectX::SpriteBatch>                         m_sprite_batch_          = nullptr;
		std::unique_ptr<DirectX::ResourceUploadBatch>                 m_resource_upload_batch_ = nullptr;
		std::unique_ptr<DirectX::SpriteBatchPipelineStateDescription> m_sprite_pipeline_state_ = nullptr;
		std::unique_ptr<DirectX::EffectPipelineStateDescription>      m_effect_pipeline_state_ = nullptr;
		std::unique_ptr<DirectX::BasicEffect>                         m_basic_effect_          = nullptr;
		std::unique_ptr<DirectX::RenderTargetState>                   m_render_target_state_   = nullptr;
		std::unique_ptr<DirectX::GraphicsMemory>                      m_graphics_memory_       = nullptr;


		std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_primitive_batch_ =
				nullptr;

		std::vector<std::function<void()>> m_sprite_batch_callbacks_;
		std::vector<std::function<void()>> m_primitive_batch_callbacks_;

		D3D12_CPU_DESCRIPTOR_HANDLE m_previous_handle_;
	};
} // namespace Engine::Managers
