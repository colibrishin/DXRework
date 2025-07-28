#pragma once
#include <directx/d3d12.h>
#include <directxtk12/SpriteFont.h>
#include <directxtk12/ResourceUploadBatch.h>
#include <directxtk12/DescriptorHeap.h>

#include "Mesh.h"

namespace Engine
{
	struct ENGINE_D3D12GRAPHICINTERFACE_API D3D12PrimitiveFont : public IFont
	{
        ~D3D12PrimitiveFont() override;
		D3D12PrimitiveFont() = default;
		void Generate(const Resources::Font* font) override;
		void Render(const std::string_view text, const Vector2& position, const Color& color, const float rotation_rad, const Vector2& scale) override;

	private:
		Unique<DirectX::SpriteFont> m_native_font_ = nullptr;
		Unique<DirectX::DescriptorHeap> m_heap_ = nullptr;
	};
}
