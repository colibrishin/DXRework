#include "../Public/ShadowTexture.h"

#if defined(USE_DX12)
#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#endif

namespace Engine::Resources
{
	void ShadowTexture::FixedUpdate(const float& dt)
	{
		Texture2D::FixedUpdate(dt);
	}

	void ShadowTexture::Initialize()
	{
		Texture2D::Initialize();
	}

	void ShadowTexture::PostUpdate(const float& dt)
	{
		Texture2D::PostUpdate(dt);
	}

	void ShadowTexture::PreUpdate(const float& dt)
	{
		Texture2D::PreUpdate(dt);
	}

	void ShadowTexture::Update(const float& dt)
	{
		Texture2D::Update(dt);
	}

	void ShadowTexture::OnSerialized()
	{
		Texture2D::OnSerialized();
	}

	void ShadowTexture::OnDeserialized()
	{
		Texture2D::OnDeserialized();
	}

	eResourceType ShadowTexture::GetResourceType() const
	{
		return RES_T_SHADOW_TEX;
	}

	UINT ShadowTexture::GetDepth() const
	{
		return Texture2D::GetDepth();
	}

	UINT ShadowTexture::GetHeight() const
	{
		return Texture2D::GetHeight();
	}

	UINT64 ShadowTexture::GetWidth() const
	{
		return Texture2D::GetWidth();
	}

	void ShadowTexture::Clear(ID3D12GraphicsCommandList1* cmd) const
	{
		const auto& dsv_trans = CD3DX12_RESOURCE_BARRIER::Transition
				(GetRawResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		cmd->ResourceBarrier(1, &dsv_trans);

		cmd->ClearDepthStencilView
				(
				 m_dsv_->GetCPUDescriptorHandleForHeapStart(),
				 D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
				 1.0f,
				 0,
				 0,
				 nullptr
				);

		const auto& dsv_trans_back = CD3DX12_RESOURCE_BARRIER::Transition
				(GetRawResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COMMON);

		cmd->ResourceBarrier(1, &dsv_trans_back);
	}

	void ShadowTexture::Unload_INTERNAL()
	{
		Texture2D::Unload_INTERNAL();
	}
}
