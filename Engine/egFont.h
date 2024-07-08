#pragma once
#include "egD3Device.hpp"
#include "egResource.h"
#include "egResourceManager.hpp"
#include "egToolkitAPI.h"

namespace Engine::Resources
{
	class Font : public Abstract::Resource
	{
	public:
		RESOURCE_T(RES_T_FONT)

		Font(const std::filesystem::path& path);
		~Font() override = default;

		void Initialize() override;

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PostUpdate(const float& dt) override;
		void FixedUpdate(const float& dt) override;

		void SetText(const std::string& text);
		void SetPosition(const Vector2& position);
		void SetColor(const Vector4& color);
		void SetRotation(float radian);
		void SetScale(float scale);
		void ChangeFont(const std::filesystem::path& path);

		void OnSerialized() override;
		void OnDeserialized() override;

		RESOURCE_SELF_INFER_GETTER(Font)
		RESOURCE_SELF_INFER_CREATE(Font)

	protected:
		Font(); // for serialization
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		SERIALIZE_DECL

		Vector2     m_position_;
		Vector4     m_color_;
		float       m_rotation_radian_;
		float       m_scale_;
		std::string m_text_;

		bool m_lazy_reload_;

		std::unique_ptr<SpriteFont> m_font_;
		DirectX::DescriptorHeap     m_heap_;
	};
} // namespace Engine::Resources

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Font)
