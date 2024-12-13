#pragma once
#include <array>

#if defined(USE_DX12)
#include <directx/d3d12.h>
#endif

#include "Source/Runtime/Resources/Shader/Public/Shader.hpp"
#include "Source/Runtime/Core/StructuredBuffer.h"
#include "Source/Runtime/Managers/ResourceManager/Public/ResourceManager.hpp"

namespace Engine::Resources
{
	class COMPUTESHADER_API ComputeShader : public Shader
	{
	public:
		~ComputeShader() override = default;

		void                SetGroup(const std::array<UINT, 3>& group);
		std::array<uint32_t, 3> GetThread() const;
		void Dispatch(Graphics::SBs::LocalParamSB& param);

		template <typename T, typename CSLock = std::enable_if_t<std::is_base_of_v<ComputeShader, T>>>
		static boost::weak_ptr<T> Create()
		{
			const auto& v = boost::make_shared<T>();
			v->Initialize();
			v->Load();
			Managers::ResourceManager::GetInstance().AddResource(v);
			return v;
		}

		RESOURCE_SELF_INFER_GETTER_DECL(ComputeShader)

	protected:
		ComputeShader(const std::string& name, const std::filesystem::path& path, const std::array<uint32_t, 3>& thread);

		virtual void preDispatch() = 0;
		virtual void postDispatch() = 0;

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

		ComputeShader();

		ComputePrimitiveShader* m_primitive_shader_;

		std::array<uint32_t, 3> m_thread_;
		std::array<uint32_t, 3> m_group_;
	};
} // namespace Engine::Resources
