#pragma once
#include <array>

#include "Source/Runtime/Resources/Shader/Public/Shader.h"
#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.h"

#include "ComputeShader.generated.h"

namespace Engine::Resources
{
	ECLASS(resource, abstract, serialize)
	class ENGINE_COMPUTESHADER_API ComputeShader : public Shader
	{
		GENERATE_BODY
	public:
		~ComputeShader() override = default;

		std::array<uint32_t, 3> GetThread() const;
		void Dispatch(const GraphicInterfaceContextPrimitive* context, const UINT group_count[ 3 ], Graphics::SBs::LocalParamSB& param);

		[[nodiscard]] ComputePrimitiveShader& GetComputePrimitiveShader() const;

	protected:
		ComputeShader(const std::filesystem::path& path, const std::array<uint32_t, 3>& thread);

		virtual void preDispatch(const GraphicInterfaceContextPrimitive* context, Graphics::SBs::LocalParamSB& param) = 0;
		virtual void postDispatch(const GraphicInterfaceContextPrimitive* context, Graphics::SBs::LocalParamSB& param) = 0;

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
		
		EPROPERTY()
		std::array<uint32_t, 3> m_thread_;

		Unique<ComputePrimitiveShader> m_primitive_shader_;
	};
} // namespace Engine::Resources
