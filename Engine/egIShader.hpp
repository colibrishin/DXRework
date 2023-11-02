#pragma once
#include <filesystem>

#include "egCommon.hpp"
#include "egEntity.hpp"
#include "egResource.hpp"

namespace Engine::Graphic
{
	using Microsoft::WRL::ComPtr;

	class IShader : public Abstract::Resource
	{
	public:
		IShader(const std::wstring& name, const std::filesystem::path& path);
		~IShader() override = default;

		ID3D11Buffer* GetBuffer() const { return m_buffer_.Get(); }

		void Render() override;

		virtual void SetShaderType() = 0;
		eShaderType GetType() const { return m_type_; }

	protected:
		eShaderType m_type_;

	private:
		std::filesystem::path m_path_;
		ComPtr<ID3D11Buffer> m_buffer_;
	};
}
