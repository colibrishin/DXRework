#include "pch.hpp"
#include "egRenderPipeline.hpp"

#include <filesystem>

#include "egD3Device.hpp"
#include "egManagerHelper.hpp"
#include "egToolkitAPI.hpp"

#include "egVertexShaderInternal.hpp"
#include "egShader.hpp"


namespace Engine::Manager::Graphics
{
	void RenderPipeline::SetWorldMatrix(const TransformBuffer& matrix)
	{
		m_transform_buffer_data_.SetData(GetD3Device().GetContext(), matrix);
		GetD3Device().BindConstantBuffer(m_transform_buffer_data_, CB_TYPE_TRANSFORM, SHADER_VERTEX);
	}

	void RenderPipeline::SetPerspectiveMatrix(const PerspectiveBuffer& matrix)
	{
		m_wvp_buffer_data_.SetData(GetD3Device().GetContext(), matrix);
		GetD3Device().BindConstantBuffer(m_wvp_buffer_data_, CB_TYPE_WVP, SHADER_VERTEX);
	}

	void RenderPipeline::SetLight(UINT id, const Matrix& world, const Matrix& vp, const Color& color)
	{
		m_light_buffer_.world[id] = world;
		m_light_buffer_.vp[id] = vp;
		m_light_buffer_.color[id] = color;
	}

	void RenderPipeline::SetSpecularPower(float power)
	{
		m_specular_buffer_.specular_power = power;
		m_specular_buffer_data_.SetData(GetD3Device().GetContext(), m_specular_buffer_);
		GetD3Device().BindConstantBuffer(m_specular_buffer_data_, CB_TYPE_SPECULAR, SHADER_PIXEL);
	}

	void RenderPipeline::SetSpecularColor(const Color& color)
	{
		m_specular_buffer_.specular_color = color;
		m_specular_buffer_data_.SetData(GetD3Device().GetContext(), m_specular_buffer_);
		GetD3Device().BindConstantBuffer(m_specular_buffer_data_, CB_TYPE_SPECULAR, SHADER_PIXEL);
	}

	void RenderPipeline::BindLightBuffer()
	{
		m_light_buffer_data.SetData(GetD3Device().GetContext(), m_light_buffer_);

		GetD3Device().BindConstantBuffer(m_light_buffer_data, CB_TYPE_LIGHT, SHADER_VERTEX);
		GetD3Device().BindConstantBuffer(m_light_buffer_data, CB_TYPE_LIGHT, SHADER_PIXEL);
		GetD3Device().BindConstantBuffer(m_light_buffer_data, CB_TYPE_LIGHT, SHADER_GEOMETRY);
	}

	void RenderPipeline::SetTopology(const D3D11_PRIMITIVE_TOPOLOGY& topology)
	{
		GetD3Device().GetContext()->IASetPrimitiveTopology(topology);
	}

	
	void RenderPipeline::GetCascadeShadow(const Vector3& light_dir, Vector4 position[3], Matrix view[3], Matrix projection[3], Vector4 clip[3]) const
	{
		// https://cutecatgame.tistory.com/6

		if (const auto scene = GetSceneManager().GetActiveScene().lock())
		{
			if (const auto camera = scene->GetMainCamera().lock())
			{
				const auto view_inv = camera->GetViewMatrix().Invert();
				const float fov = g_fov;
				const float aspect = GetD3Device().GetAspectRatio();
				const float near_plane = g_screen_near;
				const float far_plane = g_screen_far;

				const float tan_hf_v = std::tanf(XMConvertToRadians(fov / 2.f));
				const float tan_hf_h = tan_hf_v * aspect;

				const float cascadeEnds[]
				{
					near_plane, 6.f, 18.f, far_plane
				};

				// for cascade shadow mapping, total 3 parts are used.
				// (near, 6), (6, 18), (18, far)
				for (auto i = 0; i < g_max_shadow_cascades; ++i)
				{
					const float xn = cascadeEnds[i] * tan_hf_h;
					const float xf = cascadeEnds[i + 1] * tan_hf_h;

					const float yn = cascadeEnds[i] * tan_hf_v;
					const float yf = cascadeEnds[i + 1] * tan_hf_v;

					// frustum = near points 4 + far points 4
					Vector4 current_corner[8] =
					{
						// near plane
						{xn, yn, cascadeEnds[i], 1.f},
						{-xn, yn, cascadeEnds[i], 1.f},
						{xn, -yn, cascadeEnds[i], 1.f},
						{-xn, -yn, cascadeEnds[i], 1.f},

						// far plane
						{xf, yf, cascadeEnds[i + 1], 1.f},
						{-xf, yf, cascadeEnds[i + 1], 1.f},
						{xf, -yf, cascadeEnds[i + 1], 1.f},
						{-xf, -yf, cascadeEnds[i + 1], 1.f}
					};

					Vector4 center{};

					// Move to world space
					for (auto& corner : current_corner)
					{
						corner = Vector4::Transform(corner, view_inv);
						center += corner;
					}

					// Get center by averaging
					center /= 8.f;

					float radius = 0.f;
					for (auto& corner : current_corner)
					{
						float distance = Vector4::Distance(center, corner);
						radius = std::max(radius, distance);
					}
					
					radius = std::ceil(radius * 16.f) / 16.f;

					Vector3 maxExtent = Vector3{radius, radius, radius};
					Vector3 minExtent = -maxExtent;

					position[i] = center + (light_dir * minExtent.z);

					view[i] = XMMatrixLookAtLH(position[i], Vector3(center), Vector3::Up);

					const Vector3 cascadeExtents = maxExtent - minExtent;

					projection[i] = XMMatrixOrthographicOffCenterLH(minExtent.x, maxExtent.x, minExtent.y, maxExtent.y, 0.f, cascadeExtents.z);

					clip[i] = Vector4{0.f, 0.f, cascadeEnds[i + 1], 1.f};
					clip[i] = Vector4::Transform(clip[i], camera->GetProjectionMatrix()); // use z axis
				}
			}
		}
	}
	*/

	void RenderPipeline::SetWireframeState() const
	{
		GetD3Device().GetContext()->RSSetState(RenderPipeline::m_rasterizer_state_wire_.Get());
	}

	void RenderPipeline::SetFillState() const
	{
		GetD3Device().GetContext()->RSSetState(RenderPipeline::m_rasterizer_state_.Get());
	}

	void RenderPipeline::BindVertexBuffer(ID3D11Buffer* buffer)
	{
		constexpr UINT stride = sizeof(VertexElement);
		constexpr UINT offset = 0;
		GetD3Device().GetContext()->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
	}

	void RenderPipeline::BindIndexBuffer(ID3D11Buffer* buffer)
	{
		GetD3Device().GetContext()->IASetIndexBuffer(buffer, DXGI_FORMAT_R32_UINT, 0);
	}

	void RenderPipeline::UpdateBuffer(ID3D11Buffer* buffer, const void* data, size_t size)
	{
		GetD3Device().UpdateBuffer(size, data, buffer);
	}

	void RenderPipeline::BindResource(eShaderResource resource, ID3D11ShaderResourceView* texture)
	{
		GetD3Device().GetContext()->PSSetShaderResources(resource, 1, &texture);
	}

	void RenderPipeline::Initialize()
	{
		GetD3Device().CreateConstantBuffer(m_wvp_buffer_data_);
		GetD3Device().CreateConstantBuffer(m_transform_buffer_data_);
		GetD3Device().CreateConstantBuffer(m_light_buffer_data);
		GetD3Device().CreateConstantBuffer(m_specular_buffer_data_);

		PrecompileShaders();
		InitializeSamplers();

		GetD3Device().CreateBlendState(m_blend_state_.GetAddressOf());
		GetD3Device().CreateRasterizer(m_rasterizer_state_.GetAddressOf(), D3D11_FILL_SOLID);
		GetD3Device().CreateRasterizer(m_rasterizer_state_wire_.GetAddressOf(), D3D11_FILL_WIREFRAME);
		GetD3Device().CreateDepthStencilState(m_depth_stencil_state_.GetAddressOf());

		Engine::GetRenderPipeline().SetSpecularColor({0.5f, 0.5f, 0.5f, 1.0f});
		Engine::GetRenderPipeline().SetSpecularPower(100.0f);
	}

	void RenderPipeline::SetShader(Graphic::IShader* shader)
	{
		switch (shader->GetType())
		{
		case SHADER_VERTEX:
			GetD3Device().BindShader(reinterpret_cast<Graphic::VertexShader*>(shader));
			break;
		case SHADER_PIXEL:
			GetD3Device().BindShader(reinterpret_cast<Graphic::PixelShader*>(shader));
			break;
		case SHADER_GEOMETRY:
			GetD3Device().BindShader(reinterpret_cast<Graphic::GeometryShader*>(shader));
			break;
		case SHADER_COMPUTE:
			GetD3Device().BindShader(reinterpret_cast<Graphic::ComputeShader*>(shader));
			break;
		case SHADER_HULL:
			GetD3Device().BindShader(reinterpret_cast<Graphic::HullShader*>(shader));
			break;
		case SHADER_DOMAIN:
			GetD3Device().BindShader(reinterpret_cast<Graphic::DomainShader*>(shader));
			break;
		default: 
			assert(nullptr);
		}
	}

	void RenderPipeline::PrecompileShaders()
	{
		for (const auto& file : std::filesystem::directory_iterator("./"))
		{
			if (file.path().extension() == ".hlsl")
			{
				const auto prefix = file.path().filename().string();
				const auto filename_without_extension = prefix.substr(0, prefix.find_last_of("."));

				if (prefix.starts_with("vs"))
				{
					boost::shared_ptr<Graphic::Shader<ID3D11VertexShader>> shader = boost::make_shared<Graphic::VertexShader>(filename_without_extension, file);

					shader->Load();
					GetResourceManager().AddResource(filename_without_extension, shader);
				}
				else if (prefix.starts_with("ps"))
				{
					boost::shared_ptr<Graphic::PixelShader> shader = boost::make_shared<Graphic::PixelShader>(
						filename_without_extension, file);

					shader->Load();
					GetResourceManager().AddResource(filename_without_extension, shader);
				}
				else if (prefix.starts_with("gs"))
				{
					boost::shared_ptr<Graphic::GeometryShader> shader = boost::make_shared<Graphic::GeometryShader>(
						filename_without_extension, file);

					shader->Load();
					GetResourceManager().AddResource(filename_without_extension, shader);
				}
				else if (prefix.starts_with("cs"))
				{
					boost::shared_ptr<Graphic::ComputeShader> shader = boost::make_shared<Graphic::ComputeShader>(
						filename_without_extension, file);

					shader->Load();
					GetResourceManager().AddResource(filename_without_extension, shader);
				}
				else if (prefix.starts_with("hs"))
				{
					boost::shared_ptr<Graphic::HullShader> shader = boost::make_shared<Graphic::HullShader>(
						filename_without_extension, file);

					shader->Load();
					GetResourceManager().AddResource(filename_without_extension, shader);
				}
				else if (prefix.starts_with("ds"))
				{
					boost::shared_ptr<Graphic::DomainShader> shader = boost::make_shared<Graphic::DomainShader>(
						filename_without_extension, file);

					shader->Load();
					GetResourceManager().AddResource(filename_without_extension, shader);
				}
			}
		}
	}

	void RenderPipeline::InitializeSamplers()
	{
		const auto sampler = GetToolkitAPI().GetCommonStates()->LinearWrap();

		s_sampler_state_[SHADER_VERTEX] = sampler;
		GetD3Device().BindSampler(s_sampler_state_[SHADER_VERTEX], SHADER_VERTEX);

		s_sampler_state_[SHADER_PIXEL] = sampler;
		GetD3Device().BindSampler(s_sampler_state_[SHADER_PIXEL], SHADER_PIXEL);

		s_sampler_state_[SHADER_GEOMETRY] = sampler;
		GetD3Device().BindSampler(s_sampler_state_[SHADER_GEOMETRY], SHADER_GEOMETRY);

		s_sampler_state_[SHADER_COMPUTE] = sampler;
		GetD3Device().BindSampler(s_sampler_state_[SHADER_COMPUTE], SHADER_COMPUTE);

		s_sampler_state_[SHADER_HULL] = sampler;
		GetD3Device().BindSampler(s_sampler_state_[SHADER_HULL], SHADER_HULL);

		s_sampler_state_[SHADER_DOMAIN] = sampler;
		GetD3Device().BindSampler(s_sampler_state_[SHADER_DOMAIN], SHADER_DOMAIN);
	}

	void RenderPipeline::PreRender(const float& dt)
	{
		// ** overriding DirectXTK common state
		GetD3Device().GetContext()->RSSetState(RenderPipeline::m_rasterizer_state_.Get());
		GetD3Device().GetContext()->OMSetBlendState(RenderPipeline::m_blend_state_.Get(), nullptr, 0xFFFFFFFF);
		GetD3Device().GetContext()->OMSetDepthStencilState(RenderPipeline::m_depth_stencil_state_.Get(), 1);

		GetD3Device().GetContext()->IASetInputLayout(GetRenderPipeline().m_input_layout_.Get());
	}

	void RenderPipeline::DrawIndexed(UINT index_count)
	{
		GetD3Device().GetContext()->DrawIndexed(index_count, 0, 0);
	}
}
