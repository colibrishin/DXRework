#include "D3D12PrimitiveFont.h"
#include "Font.h"
#include "ToolkitAPI.h"
#include "D3D12GraphicInterface.h"

Engine::D3D12PrimitiveFont::~D3D12PrimitiveFont()
{ }

void Engine::D3D12PrimitiveFont::Generate( const Resources::Font* font )
{
	IGraphicAPI& gi = g_graphic_accessor.GetInterface();
	D3D12GraphicInterface& dgi = static_cast<D3D12GraphicInterface&>(gi);
	ID3D12Device* dev = static_cast<ID3D12Device*>(gi.GetNativeInterface());

	if (!m_heap_)
	{
		m_heap_ = std::make_unique<DirectX::DescriptorHeap>(dev, 1);
	}

	DirectX::ResourceUploadBatch batch(dev);
	batch.Begin();

	m_native_font_ = std::make_unique<DirectX::SpriteFont>(
		dev, 
		batch, 
		font->GetPath().c_str(), 
		m_heap_->GetFirstCpuHandle(), 
		m_heap_->GetFirstGpuHandle());
	
	const auto& token = batch.End(dgi.GetCommandTask().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));
	token.wait();

	SetNativeFont(m_native_font_.get());
}

void Engine::D3D12PrimitiveFont::Render(
	const std::string_view text, 
	const Vector2& position, 
	const Color& color, 
	const float rotation_rad, 
	const Vector2& scale)
{
	Managers::ToolkitAPI::GetInstance().AppendSpriteBatch([this, text, position, color, rotation_rad, scale]()
		{
			m_native_font_->DrawString(
				Managers::ToolkitAPI::GetInstance().GetSpriteBatch(),
				text.data(),
				position,
				color,
				rotation_rad,
				scale);
		});
}
