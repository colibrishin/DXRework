#pragma once
#include <filesystem>

#include "egCommon.hpp"
#include "egEntity.hpp"

namespace Engine::Graphic
{
	class IShader : public Abstract::Entity
	{
	public:
		IShader(const std::wstring& name, const std::filesystem::path& path);
		~IShader() override = default;

		std::filesystem::path GetPath() const { return m_path_; }
		ID3D11Buffer* GetBuffer() const { return m_buffer_.Get(); }

		void Initialize() override;
		void PreUpdate() override;
		void Update() override;

		virtual void SetShaderType() = 0;
		eShaderType GetType() const { return m_type_; }

	protected:
		eShaderType m_type_;

	private:
		std::filesystem::path m_path_;
		ComPtr<ID3D11Buffer> m_buffer_;
	};

	inline IShader::IShader(const std::wstring& name, const std::filesystem::path& path)
	{
		SetName(name);
		m_path_ = path;
	}

	inline void IShader::Initialize()
	{
	}

	inline void IShader::PreUpdate()
	{
	}

	inline void IShader::Update()
	{
	}
}
