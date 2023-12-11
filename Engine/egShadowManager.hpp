#pragma once
#include "egManager.hpp"

namespace Engine::Manager::Graphics
{
	constexpr float placeholder = 0.f;

	class ShadowManager : public Abstract::Singleton<ShadowManager>
	{
	private:
		struct Subfrusta
		{
			Vector4 corners[8];
		};

	public:
		ShadowManager(SINGLETON_LOCK_TOKEN) : Abstract::Singleton<ShadowManager>() {}
		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void FixedUpdate(const float& dt) override;

		void SetCascadeShadow(const Vector3& light_dir);

	private:
		void CreateFrusta(const Matrix& projection, float start, float end, Subfrusta& subfrusta) const;

		void BuildShadowMap(Scene& scene) const;

		boost::shared_ptr<Graphic::VertexShader> m_vs_stage1;
		boost::shared_ptr<Graphic::GeometryShader> m_gs_stage1;
		boost::shared_ptr<Graphic::PixelShader> m_ps_stage1;

		Subfrusta m_subfrusta_[3];
		CascadeShadowBuffer m_shadow_buffer_;
	};
}
