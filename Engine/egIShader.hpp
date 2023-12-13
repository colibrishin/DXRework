#pragma once
#include <filesystem>

#include "egCommon.hpp"
#include "egResource.hpp"

namespace Engine::Graphic
{
	using Microsoft::WRL::ComPtr;

	class IShader : public Abstract::Resource
	{
	public:
		IShader(const EntityName& name, const std::filesystem::path& path);
		~IShader() override = default;

		ID3D11Buffer* GetBuffer() const { return m_buffer_.Get(); }

		void Render(const float& dt) override;
		eShaderType GetType() const { return m_type_; }

	protected:
		virtual void SetShaderType() = 0;
		eShaderType m_type_;

	private:
		SERIALIZER_ACCESS

		ComPtr<ID3D11Buffer> m_buffer_;
	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Graphic::IShader)