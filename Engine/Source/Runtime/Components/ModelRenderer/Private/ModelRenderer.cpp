#include "../Public/ModelRenderer.h"

namespace Engine::Components
{
	COMP_CLONE_IMPL(ModelRenderer)

	ModelRenderer::ModelRenderer(const Weak<Engine::Abstracts::ObjectBase>& owner)
		: RenderComponent(RENDER_COM_T_MODEL, owner) {}

	void ModelRenderer::PreUpdate(const float& dt) {}

	void ModelRenderer::Update(const float& dt) {}

	void ModelRenderer::FixedUpdate(const float& dt) {}

	void ModelRenderer::PostUpdate(const float& dt)
	{
		Component::PostUpdate(dt);
	}

	ModelRenderer::ModelRenderer()
		: RenderComponent(RENDER_COM_T_MODEL, {}) {}
}

Engine::Graphics::SBs::InstanceModelSB::InstanceModelSB()
{
	SetFrame(0.f);
	SetAnimDuration(0);
	SetAnimIndex(0);
	SetNoAnim(false);
	SetWorld(Matrix::Identity);
}

void Engine::Graphics::SBs::InstanceModelSB::SetFrame(const float frame)
{
	SetParam(0, frame);
}

void Engine::Graphics::SBs::InstanceModelSB::SetAnimDuration(const UINT duration)
{
	SetParam(0, static_cast<int>(duration));
}

void Engine::Graphics::SBs::InstanceModelSB::SetAnimIndex(const UINT index)
{
	SetParam(1, static_cast<int>(index));
}

void Engine::Graphics::SBs::InstanceModelSB::SetNoAnim(const bool no_anim)
{
	SetParam(2, static_cast<int>(no_anim));
}

void Engine::Graphics::SBs::InstanceModelSB::SetAtlasX(const UINT x)
{
	SetParam(3, static_cast<int>(x));
}

void Engine::Graphics::SBs::InstanceModelSB::SetAtlasY(const UINT y)
{
	SetParam(4, static_cast<int>(y));
}

void Engine::Graphics::SBs::InstanceModelSB::SetAtlasW(const UINT w)
{
	SetParam(5, static_cast<int>(w));
}

void Engine::Graphics::SBs::InstanceModelSB::SetAtlasH(const UINT h)
{
	SetParam(6, static_cast<int>(h));
}

void Engine::Graphics::SBs::InstanceModelSB::SetRepeat(const bool repeat)
{
	SetParam(7, static_cast<int>(repeat));
}

void Engine::Graphics::SBs::InstanceModelSB::SetWorld(const Matrix& world)
{
	SetParam(0, world);
}
