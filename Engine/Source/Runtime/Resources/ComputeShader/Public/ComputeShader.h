#pragma once
#include <array>

#include "Source/Runtime/Resources/Shader/Public/Shader.h"
#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.h"

#include "ComputeShader.generated.h"

namespace Engine::Resources
{
	ECLASS()
	class ENGINE_COMPUTESHADER_API ComputeShader : public Shader
	{
		GENERATE_BODY
	public:
		~ComputeShader() override = default;

		std::array<uint32_t, 3> GetThread() const;
		void Dispatch(const GraphicInterfaceContextPrimitive* context, const UINT group_count[3], const Graphics::SBs::LocalParamSB& param) const;

		template <typename T, typename CSLock = std::enable_if_t<std::is_base_of_v<ComputeShader, T>>>
		static Weak<T> Create()
		{
			const auto& v = boost::make_shared<T>();
			v->Initialize();
			v->Load();
			Managers::ResourceManager::GetInstance().AddResource(v);
			return v;
		}

		[[nodiscard]] ComputePrimitiveShader& GetComputePrimitiveShader() const;

		RESOURCE_SELF_INFER_GETTER_DECL(ComputeShader)

	protected:
		ComputeShader(const std::string& name, const std::filesystem::path& path, const std::array<uint32_t, 3>& thread);

		virtual void preDispatch() = 0;
		virtual void postDispatch() = 0;

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
		Unique<ComputePrimitiveShader> m_primitive_shader_;
		
		EPROPERTY()
		std::array<uint32_t, 3> m_thread_;
	};
} // namespace Engine::Resources
