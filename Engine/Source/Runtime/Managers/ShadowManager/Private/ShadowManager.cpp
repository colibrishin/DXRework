#include "../Public/ShadowManager.hpp"

#include "Source/Runtime/Core/Components/Transform/Public/Transform.h"
#include "Source/Runtime/Core/Objects/Camera/Public/Camera.h"
#include "Source/Runtime/Core/Objects/Light/Public/Light.h"
#include "Source/Runtime/Core/Scene/Public/Scene.hpp"

#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"
#include "Source/Runtime/Resources/Shader/Public/Shader.hpp"
#include "Source/Runtime/Resources/ShadowTexture/Public/ShadowTexture.h"

#include "Source/Runtime/Managers/RenderPipeline/Public/Renderer.h"
#include "Source/Runtime/Managers/ResourceManager/Public/ResourceManager.hpp"
#include "Source/Runtime/Managers/SceneManager/Public/SceneManager.hpp"

namespace Engine::Managers
{
	void ShadowManager::Initialize()
	{
		m_shadow_shader_ = Resources::Shader::Create
				(
				 "cascade_shadow_stage1", "./cascade_shadow_stage1.hlsl", 
				 SHADER_DOMAIN_OPAQUE, SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
				 SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
				 SAMPLER_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
				 SHADER_SAMPLER_CLAMP | SHADER_SAMPLER_LESS_EQUAL,
				 GetDefaultRTVFormat(), TEX_FORMAT_D32_FLOAT,
				 PRIMITIVE_TOPOLOGY_TRIANGLELIST, PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
				 SAMPLER_SHADOW
				);

		// Render target for shadow map mask.
		m_shadow_map_mask_ = Resources::Texture2D::Create
				(
				 "Shadow Depth Mask Texture",
				 "",
				 {
					 .Alignment = 0,
					 .Width = CFG_CASCADE_SHADOW_TEX_WIDTH,
					 .Height = CFG_CASCADE_SHADOW_TEX_HEIGHT,
					 .DepthOrArraySize = CFG_CASCADE_SHADOW_COUNT,
					 .Format = TEX_FORMAT_R8G8B8A8_UNORM,
					 .Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
					 .MipsLevel = 1,
					 .Layout = TEX_LAYOUT_UNKNOWN,
					 .SampleDesc = {1, 0},
				 }
				);

		m_shadow_map_mask_->Load();
		m_shadow_map_mask_->Initialize();

		InitializeViewport();

		if (!m_shadow_task_)
		{
			throw std::runtime_error("No shadow task has been assigned to");
		}

		m_shadow_task_->SetShadowShader(m_shadow_shader_);
	}

	void ShadowManager::PreUpdate(const float& dt)
	{
		// Remove the expired lights just in case.
		std::erase_if
				(
				 m_lights_, [](const auto& kv)
				 {
					 return kv.second.expired();
				 }
				);
	}

	void ShadowManager::Update(const float& dt) {}

	void ShadowManager::GetLightVP(const boost::shared_ptr<Scene>& scene, std::vector<SBs::LightVPSB>& current_light_vp)
	{
		for (const auto& ptr_light : m_lights_ | std::views::values)
		{
			if (const auto light = ptr_light.lock())
			{
				const auto tr = light->GetComponent<Components::Transform>().lock();

				// Get the light direction from the light's position.
				Vector3 light_dir;
				(tr->GetWorldPosition()).Normalize(light_dir);

				if (light_dir == Vector3::Zero)
				{
					current_light_vp.push_back({});
					continue;
				}

				SBs::LightVPSB light_vp{};
				// Get the light's view and projection matrix in g_max_shadow_cascades parts.
				EvalShadowVP(scene->GetMainCamera(), light_dir, light_vp);
				current_light_vp.emplace_back(light_vp);
			}
		}
	}

	void ShadowManager::PreRender(const float& dt)
	{
		constexpr size_t light_slot = 0;

		// # Pass 1 : depth only, building shadow map

		// Build light information structured buffer.
		std::vector<SBs::LightSB> light_buffer;

		for (const auto& light : m_lights_ | std::views::values)
		{
			if (const auto locked = light.lock())
			{
				const auto tr = locked->GetComponent<Components::Transform>().lock();

				const auto world = tr->GetWorldMatrix();

				light_buffer.emplace_back
						(
						 world.Transpose(),
						 locked->GetColor(),
						 locked->GetType(),
						 locked->GetRange()
						);
			}
		}

		// Notify the number of lights to the shader.
		Managers::RenderPipeline::GetInstance().SetParam<int>(static_cast<UINT>(m_lights_.size()), light_slot);

		// If there is no light, it does not need to be updated.
		if (light_buffer.empty())
		{
			return;
		}

		if (const auto scene = Managers::SceneManager::GetInstance().GetActiveScene().lock())
		{
			std::vector<SBs::LightVPSB> current_light_vp;
			GetLightVP(scene, current_light_vp);

			// Also, if there is no light, it does not need to be updated.
			if (current_light_vp.empty())
			{
				return;
			}

			ClearShadowMaps(cmd);
			
			CheckSize<UINT>(light_buffer.size(), L"Warning: Light buffer size is too big!");
			CheckSize<UINT>(current_light_vp.size(), L"Warning: Light VP size is too big!");

			m_shadow_task_->UpdateLight(light_buffer);
			m_shadow_task_->UpdateLightVP(current_light_vp);

			UINT idx = 0;

			for (const auto& ptr_light : m_lights_ | std::views::values)
			{
				if (const auto light = ptr_light.lock())
				{
					// Render the depth of the object from the light's point of view.
					BuildShadowMap(dt, light, idx++);
				}
			}
		}
	}

	void ShadowManager::Render(const float& dt) {}

	void ShadowManager::PostRender(const float& dt) {}

	void ShadowManager::FixedUpdate(const float& dt) {}

	void ShadowManager::PostUpdate(const float& dt) {}

	void ShadowManager::Reset()
	{
		m_lights_.clear();
		m_shadow_texs_.clear();

		for (auto& subfrusta : m_subfrusta_)
		{
			subfrusta = {};
		}
	}

	void ShadowManager::BuildShadowMap(
		const float dt, const Strong<Objects::Light>& light, const UINT light_idx
	)
	{
		// Notify the light index to the shader.
		SBs::LocalParamSB local_param{};
		local_param.SetParam(0, static_cast<int>(light_idx));

		aligned_vector<RenderPassPrerequisiteTask*> vec;
		vec.push_back(m_viewport_task_.get());
		vec.push_back(m_shadow_task_.get());

		Managers::Renderer::GetInstance().RenderPass
				(
				 dt, true, SHADER_DOMAIN_OPAQUE, local_param,
				 vec, [](const Strong<Abstracts::ObjectBase>& obj)
				 {
					 if (obj->GetLayer() == RESERVED_LAYER_CAMERA || 
						 obj->GetLayer() == RESERVED_LAYER_UI || 
						 obj->GetLayer() == RESERVED_LAYER_ENVIRONMENT ||
					     obj->GetLayer() == RESERVED_LAYER_LIGHT || 
						 obj->GetLayer() == RESERVED_LAYER_SKYBOX)
					 {
						 return false;
					 }

					 return true;
				 }
				);

		/*
		 It only needs to render the depth of the object from the light's point of view.
				 // Swap the depth stencil to the each light's shadow map.
				 const auto& dsv = m_shadow_texs_.at(light->GetLocalID());
				 m_shadow_map_mask_.Bind(c, dsv);

				 const auto& h = wh.lock();

				 h->SetSampler(m_sampler_heap_->GetCPUDescriptorHandleForHeapStart(), SAMPLER_SHADOW);

				 c->GetList()->IASetPrimitiveTopology(m_shadow_shader_->GetTopology());
			 }, [this, light](const Weak<CommandPair>& c, const DescriptorPtr& h)
			 {
				 const auto& dsv = m_shadow_texs_.at(light->GetLocalID());

				 m_shadow_map_mask_.Unbind(c, dsv);
		*/
	}

	void ShadowManager::CreateSubfrusta(
		const Matrix& projection, float start,
		float         end, Subfrusta&   subfrusta
	)
	{
		BoundingFrustum frustum(projection);

		frustum.Near = start;
		frustum.Far  = end;

		static constexpr XMVECTORU32 vGrabY = {
			0x00000000, 0xFFFFFFFF, 0x00000000,
			0x00000000
		};
		static constexpr XMVECTORU32 vGrabX = {
			0xFFFFFFFF, 0x00000000, 0x00000000,
			0x00000000
		};

		const Vector4 rightTopV   = {frustum.RightSlope, frustum.TopSlope, 1.f, 1.f};
		const Vector4 leftBottomV = {
			frustum.LeftSlope, frustum.BottomSlope, 1.f,
			1.f
		};
		const Vector4 nearV = {frustum.Near, frustum.Near, frustum.Near, 1.f};
		const Vector4 farV  = {frustum.Far, frustum.Far, frustum.Far, 1.f};

		const Vector4 rightTopNear   = rightTopV * nearV;
		const Vector4 righTopFar     = rightTopV * farV;
		const Vector4 LeftBottomNear = leftBottomV * nearV;
		const Vector4 LeftBottomFar  = leftBottomV * farV;

		subfrusta.corners[0] = rightTopNear;
		subfrusta.corners[1] = XMVectorSelect(rightTopNear, LeftBottomNear, vGrabX);
		subfrusta.corners[2] = LeftBottomNear;
		subfrusta.corners[3] = XMVectorSelect(rightTopNear, LeftBottomNear, vGrabY);

		subfrusta.corners[4] = righTopFar;
		subfrusta.corners[5] = XMVectorSelect(righTopFar, LeftBottomFar, vGrabX);
		subfrusta.corners[6] = LeftBottomFar;
		subfrusta.corners[7] = XMVectorSelect(righTopFar, LeftBottomFar, vGrabY);
	}

	void ShadowManager::EvalShadowVP(const Weak<Objects::Camera>& ptr_cam, const Vector3& light_dir, SBs::LightVPSB& buffer)
	{
		// https://cutecatgame.tistory.com/6
		if (const auto& camera = ptr_cam.lock())
		{
			const float near_plane = CFG_SCREEN_NEAR;
			const float far_plane  = CFG_SCREEN_FAR;

			const float cascadeEnds[]{near_plane, 10.f, 80.f, far_plane};

			// for cascade shadow mapping, total 3 parts are used.
			// (near, 6), (6, 18), (18, far)
			for (auto i = 0; i < CFG_CASCADE_SHADOW_COUNT; ++i)
			{
				Subfrusta subfrusta[CFG_CASCADE_SHADOW_COUNT];

				// frustum = near points 4 + far points 4
				CreateSubfrusta
						(
						 camera->GetProjectionMatrix(), cascadeEnds[i],
						 cascadeEnds[i + 1], subfrusta[i]
						);

				const auto view_inv = camera->GetViewMatrix().Invert();

				// transform to world space
				Vector4 center{};
				for (auto& corner : subfrusta[i].corners)
				{
					corner = Vector4::Transform(corner, view_inv);
					center += corner;
				}

				// Get center by averaging
				center /= 8.f;

				float radius = 0.f;
				for (const auto& corner : subfrusta[i].corners)
				{
					float distance = Vector4::Distance(center, corner);
					radius         = std::max(radius, distance);
				}

				radius = std::ceil(radius * 16.f) / 16.f;

				auto          maxExtent      = Vector3{radius, radius, radius};
				Vector3       minExtent      = -maxExtent;
				const Vector3 cascadeExtents = maxExtent - minExtent;

				const auto pos = center + (light_dir * std::fabsf(minExtent.z));

				// DX11 uses row major matrix

				buffer.view[i] = XMMatrixTranspose
						(
						 XMMatrixLookAtLH(pos, Vector3(center), Vector3::Up)
						);

				buffer.proj[i] =
						XMMatrixTranspose
						(
						 XMMatrixOrthographicOffCenterLH
						 (
						  minExtent.x, maxExtent.x, minExtent.y,
						  maxExtent.y, 0.f,
						  cascadeExtents.z
						 )
						);

				buffer.end_clip_spaces[i] =
						Vector4{0.f, 0.f, cascadeEnds[i + 1], 1.f};
				buffer.end_clip_spaces[i] =
						Vector4::Transform
						(
						 buffer.end_clip_spaces[i],
						 camera->GetProjectionMatrix()
						); // use z axis
			}
		}
	}

	void ShadowManager::BindShadowMaps(const Weak<CommandPair>& w_cmd, const DescriptorPtr& w_heap) const
	{
		const auto&                              cmd  = w_cmd.lock();
		const auto&                              heap = w_heap.lock();
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> current_shadow_maps;

		for (const auto& buffer : m_shadow_texs_ | std::views::values)
		{
			// todo: refactoring
			const auto& srv_transition = CD3DX12_RESOURCE_BARRIER::Transition
					(
					 buffer.GetRawResoruce(),
					 D3D12_RESOURCE_STATE_COMMON,
					 D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
					);

			cmd->GetList()->ResourceBarrier(1, &srv_transition);

			current_shadow_maps.emplace_back(buffer.GetSRVDescriptor()->GetCPUDescriptorHandleForHeapStart());
		}

		// Bind the shadow map resource previously rendered to the pixel shader.
		CheckSize<UINT>(current_shadow_maps.size(), L"Warning: Shadow map size is too big!");
		heap->SetShaderResources
				(
				 RESERVED_TEX_SHADOW_MAP,
				 static_cast<UINT>(current_shadow_maps.size()),
				 current_shadow_maps
				);
	}

	void ShadowManager::UnbindShadowMaps(const Weak<CommandPair>& w_cmd) const
	{
		const auto& cmd = w_cmd.lock();

		for (const auto& buffer : m_shadow_texs_ | std::views::values)
		{
			buffer.Unbind(cmd, BIND_TYPE_SRV);
		}
	}

	void ShadowManager::RegisterLight(const Weak<Objects::Light>& light)
	{
		if (const auto locked = light.lock())
		{
			m_lights_[locked->GetLocalID()] = light;
			InitializeShadowBuffer(locked->GetLocalID());
		}
	}

	void ShadowManager::UnregisterLight(const Weak<Objects::Light>& light)
	{
		if (const auto locked = light.lock())
		{
			m_lights_.erase(locked->GetLocalID());
			m_shadow_texs_.erase(locked->GetLocalID());
		}
	}

	void ShadowManager::InitializeShadowBuffer(const LocalActorID id)
	{
		m_shadow_texs_[id] = Resources::ShadowTexture::Create("Shadow texture", "", {});
		m_shadow_texs_[id]->Load();
	}

	ShadowManager::~ShadowManager() { }

	void ShadowManager::InitializeViewport()
	{
		m_viewport_ = 
		{
			.topLeftX = 0,
			.topLeftY = 0,
			.width = CFG_CASCADE_SHADOW_TEX_WIDTH,
			.height = CFG_CASCADE_SHADOW_TEX_HEIGHT,
			.minDepth = 0.f,
			.maxDepth = 1.f 
		};

		m_viewport_task_->SetViewport(m_viewport_);
	}

	void ShadowManager::ClearShadowMaps(const Weak<CommandPair>& w_cmd)
	{
		const auto& cmd = w_cmd.lock();

		for (auto& tex : m_shadow_texs_ | std::views::values)
		{
			tex->Clear(cmd->GetList());
		}

		constexpr float clear_color[4] = {0.f, 0.f, 0.f, 1.f};

		const auto& command_to_rtv = CD3DX12_RESOURCE_BARRIER::Transition
				(
				 m_shadow_map_mask_.GetRawResoruce(),
				 D3D12_RESOURCE_STATE_COMMON,
				 D3D12_RESOURCE_STATE_RENDER_TARGET
				);

		const auto& rtv_to_common = CD3DX12_RESOURCE_BARRIER::Transition
				(
				 m_shadow_map_mask_.GetRawResoruce(),
				 D3D12_RESOURCE_STATE_RENDER_TARGET,
				 D3D12_RESOURCE_STATE_COMMON
				);

		cmd->GetList()->ResourceBarrier(1, &command_to_rtv);

		cmd->GetList()->ClearRenderTargetView
				(
				 m_shadow_map_mask_.GetRTVDescriptor()->GetCPUDescriptorHandleForHeapStart(),
				 clear_color,
				 0,
				 nullptr
				);

		cmd->GetList()->ResourceBarrier(1, &rtv_to_common);
	}
} // namespace Engine::Managers

namespace Engine
{
	void ShadowRenderPrerequisiteTask::SetShadowShader(const Strong<Resources::Shader>& shader)
	{
		m_shadow_shader_ = shader;
	}

	void ShadowRenderPrerequisiteTask::UpdateLight(const std::vector<Graphics::SBs::LightSB>& sb)
	{
		m_lights_ = sb;
		m_lazy_ = true;
	}

	void ShadowRenderPrerequisiteTask::UpdateLightVP(const std::vector<Graphics::SBs::LightVPSB>& sb)
	{
		m_light_vps_ = sb;
		m_lazy_ = true;
	}

	bool ShadowRenderPrerequisiteTask::IsLazy() const
	{
		return m_lazy_;
	}

	void ShadowRenderPrerequisiteTask::FlipLazy()
	{
		m_lazy_ = !m_lazy_;
	}
}
