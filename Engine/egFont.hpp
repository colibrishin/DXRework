#pragma once
#include "egD3Device.hpp"
#include "egResource.hpp"
#include "egToolkitAPI.hpp"

namespace Engine::Resources
{
	class Font : public Abstract::Resource
	{
	public:
		Font(const std::filesystem::path& path);
		~Font() override = default;

		void Initialize() override;

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void FixedUpdate(const float& dt) override;

		void SetText(const std::wstring& text) { m_text_ = text; }
		void SetPosition(const Vector2& position) { m_position_ = position; }
		void SetColor(const Vector4& color) { m_color_ = color; }
		void SetRotation(const float radian) { m_rotation_radian_ = radian; }
		void SetScale(const float scale) { m_scale_ = scale; }

		void ChangeFont(const std::filesystem::path& path) { SetPath(path); m_lazy_reload_ = true; }

	protected:
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		bool m_lazy_reload_;

		Vector2 m_position_;
		Vector4 m_color_;
		float m_rotation_radian_;
		float m_scale_;
		std::wstring m_text_;

		std::unique_ptr<SpriteFont> m_font_;

	};
}
