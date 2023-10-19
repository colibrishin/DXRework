#pragma once
#include <filesystem>

#include "egEntity.hpp"
#include "egRenderable.hpp"
#include "egRenderPipeline.hpp"
#include "egResource.hpp"

namespace Engine::Graphic
{
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

	inline IShader::IShader(const std::wstring& name, const std::filesystem::path& path): Resource(path, RESOURCE_PRIORITY_SHADER)
	{
		SetName(name);
		m_path_ = path;
	}

	inline void IShader::Render()
	{
		Graphic::RenderPipeline::SetShader(this);
	}
}
