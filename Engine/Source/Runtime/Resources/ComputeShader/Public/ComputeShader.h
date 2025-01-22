#pragma once
#include <array>

#include "Source/Runtime/Resources/Shader/Public/Shader.hpp"
#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.hpp"

namespace Engine 
{
	struct ComputePrimitiveShader;
	struct ComputeDispatchPrerequisiteTask;
}

namespace Engine::Resources
{
	class COMPUTESHADER_API ComputeShader : public Shader
	{
	public:
		~ComputeShader() override = default;

		std::array<uint32_t, 3> GetThread() const;
		void Dispatch(const GraphicInterfaceContextPrimitive* context, const UINT group_count[3], const Graphics::SBs::LocalParamSB& param) const;

		template <typename T, typename CSLock = std::enable_if_t<std::is_base_of_v<ComputeShader, T>>>
		static boost::weak_ptr<T> Create()
		{
			const auto& v = boost::make_shared<T>();
			v->Initialize();
			v->Load();
			Managers::ResourceManager::GetInstance().AddResource(v);
			return v;
		}

		void SetPrimitiveShader(ComputePrimitiveShader* shader);
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

		Unique<ComputePrimitiveShader> m_primitive_shader_;
		std::array<uint32_t, 3> m_thread_;
	};
} // namespace Engine::Resources


namespace Engine 
{
	struct COMPUTESHADER_API ComputeDispatchTask
	{
	public:
		virtual ~ComputeDispatchTask() = default;
		
		virtual void Cleanup() = 0;
	};

	struct COMPUTESHADER_API ComputePrimitiveShader
	{
	public:
		virtual ~ComputePrimitiveShader() = default;
		virtual void Generate(const Weak<Resources::ComputeShader>& shader, void* pipeline_signature) = 0;
		[[nodiscard]] void* GetNativeShader() const;

	private:
		void* m_shader_ = nullptr;
	};
}