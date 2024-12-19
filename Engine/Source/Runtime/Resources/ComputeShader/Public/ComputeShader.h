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
		void Dispatch(const UINT group_count[3], const Graphics::SBs::LocalParamSB& param);

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
		[[nodiscard]] ComputePrimitiveShader& GetPrimitiveShader() const;

		RESOURCE_SELF_INFER_GETTER_DECL(ComputeShader)

	protected:
		ComputeShader(const std::string& name, const std::filesystem::path& path, const std::array<uint32_t, 3>& thread);

		virtual void preDispatch() = 0;
		virtual void postDispatch() = 0;

		virtual void loadDerived() = 0;
		virtual void unloadDerived() = 0;

		template <typename T> requires (std::is_base_of_v<ComputeDispatchPrerequisiteTask, T>)
		void RegisterPrerequisiteTask() 
		{
			m_registered_prerequisites_.push_back(std::move(std::make_unique<T>()));

			m_unsafe_raw_prerequisites_.clear();
			for (size_t i = 0; i < m_registered_prerequisites_.size(); ++i)
			{
				m_unsafe_raw_prerequisites_.push_back(m_registered_prerequisites_[i].get());
			}
		}

	private:
		void PostUpdate(const float& dt) override;
		void PreUpdate(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void Initialize() override;

		void Load_INTERNAL() final;
		void Unload_INTERNAL() final;

		ComputeShader();

		std::unique_ptr<ComputePrimitiveShader> m_primitive_shader_;
		std::vector<std::unique_ptr<ComputeDispatchPrerequisiteTask>> m_registered_prerequisites_;
		std::vector<ComputeDispatchPrerequisiteTask*> m_unsafe_raw_prerequisites_;

		std::array<uint32_t, 3> m_thread_;
	};
} // namespace Engine::Resources


namespace Engine 
{
	struct COMPUTESHADER_API ComputeDispatchTask
	{
	public:
		virtual ~ComputeDispatchTask() = default;
		virtual void Dispatch(
			const Weak<Resources::ComputeShader>& w_shader,
			const Graphics::SBs::LocalParamSB& local_param,
			const UINT group_count[3],
			ComputeDispatchPrerequisiteTask* const* prerequisites,
			const size_t prerequisite_count) = 0;
		virtual void Cleanup() = 0;
	};

	struct COMPUTESHADER_API ComputePrimitiveShader
	{
	public:
		virtual             ~ComputePrimitiveShader() = default;
		virtual void        Generate(const Weak<Resources::ComputeShader>& shader, void* pipeline_signature) = 0;
		[[nodiscard]] void* GetComputePrimitiveShader() const;
		[[nodiscard]] ComputeDispatchTask& GetDispatchTask() const;

	protected:
		template <typename T> requires (std::is_base_of_v<ComputeDispatchTask, T>)
		void SetDispatchTask() 
		{
			m_dispatch_task = std::make_unique<T>();
		}

	private:
		std::unique_ptr<ComputeDispatchTask> m_dispatch_task;
		void* m_shader_ = nullptr;
	};

	struct COMPUTESHADER_API ComputeDispatchPrerequisiteTask
	{
		virtual ~ComputeDispatchPrerequisiteTask() = default;
		virtual void PreDispatch(ComputeDispatchTask* task_context) = 0;
		virtual void PostDispatch(ComputeDispatchTask* task_context) = 0;
	};
}