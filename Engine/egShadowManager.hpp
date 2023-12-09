#pragma once
#include "egManager.hpp"

namespace Engine::Manager::Graphics
{
	class ShadowManager : public Abstract::Singleton<ShadowManager>
	{
	public:
		ShadowManager(SINGLETON_LOCK_TOKEN) : Abstract::Singleton<ShadowManager>() {}
		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void FixedUpdate(const float& dt) override;

	private:
		void BuildShadowMap(Scene& scene) const;
		void ApplyShadow(Scene& scene) const;

		boost::shared_ptr<Graphic::VertexShader> m_vs_stage1;
		boost::shared_ptr<Graphic::GeometryShader> m_gs_stage1;
		boost::shared_ptr<Graphic::PixelShader> m_ps_stage1;

		boost::shared_ptr<Graphic::VertexShader> m_vs_stage2;
		boost::shared_ptr<Graphic::PixelShader> m_ps_stage2;
	};
}
