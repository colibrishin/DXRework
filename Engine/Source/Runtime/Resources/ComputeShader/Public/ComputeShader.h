#pragma once
#include <array>
#include "GraphicInterface.h"
#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.h"

#include "ComputeShader.generated.h"

namespace Engine::Resources
{
	ECLASS(resource, abstract, serialize)
	class ENGINE_COMPUTESHADER_API ComputeShader : public Abstracts::Resource
	{
		GENERATE_BODY
	public:
		~ComputeShader() override = default;

		ComputeShader(const ComputeShader& other);
		ComputeShader& operator=(const ComputeShader& other);

		[[nodiscard]] std::array<uint32_t, 3> GetThread() const;
		void Dispatch(const GraphicInterfaceContextPrimitive* context, const UINT group_count[ 3 ], Graphics::SBs::LocalParamSB& param, const float dt);

		[[nodiscard]] ComputePrimitiveShader& GetComputePrimitiveShader() const;

#if WITH_EDITOR
		void OnUIUpdate(UIContext* const parent, const float dt) override;
#endif

	protected:
		ComputeShader( const std::filesystem::path& path );
        void SetThread( const std::array<uint32_t, 3> &thread );

		virtual void preDispatch(const GraphicInterfaceContextPrimitive* context, Graphics::SBs::LocalParamSB& param, const float dt) = 0;
		virtual void postDispatch(const GraphicInterfaceContextPrimitive* context, Graphics::SBs::LocalParamSB& param, const float dt) = 0;

		virtual void loadDerived() = 0;
		virtual void unloadDerived() = 0;

	private:
		void PostUpdate(const float dt) override;
		void PreUpdate(const float dt) override;
		void FixedUpdate(const float dt) override;
		void Update(const float dt) override;
		void Initialize() override;

		void Load_INTERNAL() final;
		void Unload_INTERNAL() final;

		ComputeShader();

	public:
		void OnSerialized() override;

	private:
		EPROPERTY()
        std::array<uint32_t, 3> m_thread_ = { 1, 1, 1 };

		Unique<ComputePrimitiveShader> m_primitive_shader_;
	};
} // namespace Engine::Resources
