#pragma once
#include "egDXCommon.h"
#include "egMacro.h"
#include "egParticleRenderer.h"
#include "egResourceManager.hpp"
#include "egShader.hpp"
#include "egStructuredBuffer.hpp"

namespace Engine::Resources
{
	class ComputeShader : public Shader
	{
	public:
		~ComputeShader() override = default;

		void                SetGroup(const std::array<UINT, 3>& group);
		std::array<UINT, 3> GetThread() const;
		void Dispatch(
			ID3D12GraphicsCommandList1* list, const DescriptorPtr& w_heap, Graphics::SBs::LocalParamSB& param,
			Graphics::StructuredBuffer<Graphics::SBs::LocalParamSB>& buffer
		);

		template <typename T, typename CSLock = std::enable_if_t<std::is_base_of_v<ComputeShader, T>>>
		static boost::weak_ptr<T> Create()
		{
			const auto& v = boost::make_shared<T>();
			v->Initialize();
			v->Load();
			GetResourceManager().AddResource(v);
			return v;
		}

		virtual void OnImGui(const StrongParticleRenderer& pr) = 0;

		RESOURCE_SELF_INFER_GETTER(ComputeShader)

	protected:
		ComputeShader(const std::string& name, const std::filesystem::path& path, const std::array<UINT, 3>& thread);

		static Graphics::ParamBase& getParam(const StrongParticleRenderer& pr);
		static InstanceParticles&   getInstances(const StrongParticleRenderer& pr);

		virtual void preDispatch(
			ID3D12GraphicsCommandList1* list, const DescriptorPtr& heap, Graphics::SBs::LocalParamSB& param
		) = 0;
		virtual void postDispatch(
			ID3D12GraphicsCommandList1* list, const DescriptorPtr& heap, Graphics::SBs::LocalParamSB& param
		) = 0;

		virtual void loadDerived() = 0;
		virtual void unloadDerived() = 0;

	private:
		void PostUpdate(const float& dt) override;
		void PreUpdate(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void Initialize() override;

		void Load_INTERNAL() final;
		void Unload_INTERNAL() final;

		SERIALIZE_DECL
		ComputeShader();

		ComPtr<ID3DBlob> m_cs_;

		UINT m_thread_[3];
		UINT m_group_[3];
	};
} // namespace Engine::Resources

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Engine::Resources::ComputeShader)
BOOST_CLASS_EXPORT_KEY(Engine::Resources::ComputeShader)
