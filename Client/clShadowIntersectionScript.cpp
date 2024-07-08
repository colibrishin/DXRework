#include "pch.h"
#include "clShadowIntersectionScript.h"

#include <Renderer.h>
#include <egMaterial.h>
#include <egRenderPipeline.h>
#include <egSceneManager.hpp>
#include <egShader.hpp>
#include <egTransform.h>

#include "egCamera.h"
#include "egMouseManager.h"

namespace Client::Scripts
{
	SCRIPT_CLONE_IMPL(ShadowIntersectionScript)

	void ShadowIntersectionScript::Initialize()
	{
		Script::Initialize();

		m_shadow_shader_         = Resources::Shader::Get("cascade_shadow_stage1").lock();
		m_intensity_test_shader_ = Resources::Shader::Get("intensity_test").lock();

		if (const auto& cs = Resources::ComputeShader::Get("IntersectionCompute").lock())
		{
			m_intersection_compute_ = cs;
		}
		else
		{
			const auto& new_cs = Resources::ComputeShader::Create<ComputeShaders::IntersectionCompute>().lock();

			m_intersection_compute_ = new_cs;
		}

		m_tmp_shadow_depth_ = Resources::Texture2D::Create
				(
				 "tmp_shadow_depth",
				 "",
				 {
					 .Alignment = 0,
					 .Width = g_max_shadow_map_size,
					 .Height = g_max_shadow_map_size,
					 .DepthOrArraySize = 1,
					 .Format = DXGI_FORMAT_D32_FLOAT,
					 .Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
					 .MipsLevel = 1,
					 .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
					 .SampleDesc = {1, 0}
				 }
				);

		constexpr D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc
		{
			.Format = DXGI_FORMAT_R32_FLOAT,
			.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Texture2D = {0, 1, 0, 0}
		};

		m_tmp_shadow_depth_->LazySRV(srv_desc);

		m_tmp_shadow_depth_->Initialize();
		m_tmp_shadow_depth_->Load();

		for (auto& tex : m_shadow_texs_)
		{
			tex.Initialize();
			tex.Load();
		}

		for (auto& tex : m_intensity_test_texs_)
		{
			tex.Initialize();
			tex.Load();
		}

		for (auto& tex : m_intensity_position_texs_)
		{
			tex.Initialize();
			tex.Load();
		}

		for (auto& tex : m_shadow_mask_texs_)
		{
			tex.Initialize();
			tex.Load();
		}

		m_sb_light_table_ = boost::make_shared<Graphics::StructuredBuffer<
			ComputeShaders::IntersectionCompute::LightTableSB>>();

		const auto& cmd = GetD3Device().AcquireCommandPair(L"Shadow Intersection").lock();

		cmd->SoftReset();

		m_sb_light_table_->Create(cmd->GetList(), g_max_lights, nullptr);

		cmd->FlagReady();

		m_viewport_.Width    = g_max_shadow_map_size;
		m_viewport_.Height   = g_max_shadow_map_size;
		m_viewport_.MinDepth = 0.0f;
		m_viewport_.MaxDepth = 1.0f;
		m_viewport_.TopLeftX = 0.0f;
		m_viewport_.TopLeftY = 0.0f;

		m_scissor_rect_ =
		{
			.left = 0,
			.top = 0,
			.right = static_cast<LONG>(g_max_shadow_map_size),
			.bottom = static_cast<LONG>(g_max_shadow_map_size)
		};

		// also borrow the sampler state from shadow manager.
	}

	void ShadowIntersectionScript::PreUpdate(const float& dt)
	{
		for (const auto& heap : m_shadow_heaps_)
		{
			heap->Release();
		}

		m_shadow_third_pass_heap_->Release();

		m_shadow_heaps_.clear();
		m_shadow_third_pass_heap_.reset();
	}

	void ShadowIntersectionScript::Update(const float& dt)
	{
		Vector3 position;
		Vector3 dir;

		if (const auto& scene = GetSceneManager().GetActiveScene().lock())
		{
			if (const auto& camera = scene->GetMainCamera().lock())
			{
				position = camera->GetComponent<Components::Transform>().lock()->GetWorldPosition();
				dir      = camera->GetComponent<Components::Transform>().lock()->Forward();
			}
		}

		for (const auto& [key, bbox] : m_shadow_bbox_)
		{
			float dist = 0.f;

			if (bbox.Intersects(position, dir, dist))
			{
				GetDebugger().Draw(bbox, Colors::YellowGreen);
			}
			else
			{
				GetDebugger().Draw(bbox, Colors::Red);
			}
		}

		m_shadow_bbox_.clear();
	}

	void ShadowIntersectionScript::PostUpdate(const float& dt) {}

	void ShadowIntersectionScript::FixedUpdate(const float& dt) {}

	void ShadowIntersectionScript::PreRender(const float& dt) {}

	void ShadowIntersectionScript::Render(const float& dt) {}

	void ShadowIntersectionScript::FirstPass(
		const float&             dt,
		const Weak<CommandPair>& w_cmd,
		const size_t             shadow_slot,
		const StrongLayer&       lights
	)
	{
		const auto& cmd = w_cmd.lock();

		// First Pass: Shadow Map (With object and without object)
		// Draw shadow map except the object itself.
		for (int i = 0; i < lights->size(); ++i)
		{
			Graphics::SBs::LocalParamSB local_param{};
			local_param.SetParam<int>(shadow_slot, i);
			auto& local_param_mem = m_first_other_pass_local_params_.get();
			local_param_mem.SetData(cmd->GetList(), 1, &local_param);
			m_first_other_pass_local_params_.advance();

			GetRenderer().RenderPass
					(
					 dt, SHADER_DOMAIN_OPAQUE, true,
					 cmd,
					 m_shadow_heaps_,
					 m_instance_buffers_,
					 [this](const StrongObjectBase& obj)
					 {
						 if (obj->GetID() == GetOwner().lock()->GetID())
						 {
							 return false;
						 }

						 if (obj->GetLayer() == LAYER_CAMERA || obj->GetLayer() == LAYER_UI || obj->GetLayer() ==
						     LAYER_ENVIRONMENT ||
						     obj->GetLayer() == LAYER_LIGHT || obj->GetLayer() == LAYER_SKYBOX)
						 {
							 return false;
						 }

						 return true;
					 }, [this, i](const Weak<CommandPair>& wc, const DescriptorPtr& heap)
					 {
						 const auto& c = wc.lock();

						 c->GetList()->RSSetViewports(1, &m_viewport_);
						 c->GetList()->RSSetScissorRects(1, &m_scissor_rect_);

						 c->GetList()->SetPipelineState(m_shadow_shader_->GetPipelineState());
						 c->GetList()->IASetPrimitiveTopology(m_shadow_shader_->GetTopology());

						 m_shadow_texs_[i].Bind(c, heap, BIND_TYPE_DSV_ONLY, 0, 0);

						 GetShadowManager().BindShadowSampler(heap);
					 }, [this, i](const Weak<CommandPair>& wc, const DescriptorPtr& heap)
					 {
						 const auto& c = wc.lock();

						 m_shadow_texs_[i].Unbind(c, BIND_TYPE_DSV_ONLY);
					 }, {
						 &local_param_mem,
						 GetShadowManager().GetLightVPBuffer()
					 }
					);
		}

		// Render object only for picking up the exact shadow position.
		for (int i = 0; i < lights->size(); ++i)
		{
			Graphics::SBs::LocalParamSB local_param{};
			local_param.SetParam<int>(shadow_slot, i);
			auto& local_param_mem = m_first_other_pass_local_params_.get();
			local_param_mem.SetData(cmd->GetList(), 1, &local_param);
			m_first_self_pass_local_params_.advance();

			GetRenderer().RenderPass
					(
					 dt, SHADER_DOMAIN_OPAQUE, true,
					 cmd,
					 m_shadow_heaps_,
					 m_instance_buffers_,
					 [this](const StrongObjectBase& obj)
					 {
						 if (obj->GetID() != GetOwner().lock()->GetID())
						 {
							 return false;
						 }

						 if (obj->GetLayer() == LAYER_CAMERA || obj->GetLayer() == LAYER_UI || obj->GetLayer() ==
						     LAYER_ENVIRONMENT ||
						     obj->GetLayer() == LAYER_LIGHT || obj->GetLayer() == LAYER_SKYBOX)
						 {
							 return false;
						 }

						 return true;
					 }, [this, i](const Weak<CommandPair>& wc, const DescriptorPtr& h)
					 {
						 const auto& c = wc.lock();

						 c->GetList()->RSSetViewports(1, &m_viewport_);
						 c->GetList()->RSSetScissorRects(1, &m_scissor_rect_);

						 c->GetList()->SetPipelineState(m_shadow_shader_->GetPipelineState());
						 c->GetList()->IASetPrimitiveTopology(m_shadow_shader_->GetTopology());

						 GetShadowManager().BindShadowSampler(h);

						 m_shadow_mask_texs_[i].Bind(c, m_shadow_texs_[i]);
					 }, [this, i](const Weak<CommandPair>& wc, const DescriptorPtr& h)
					 {
						 const auto& c = wc.lock();

						 m_shadow_mask_texs_[i].Unbind(c, m_shadow_texs_[i]);
					 }, {
						 &local_param_mem,
						 GetShadowManager().GetLightVPBuffer()
					 }
					);
		}
	}

	void ShadowIntersectionScript::SecondPass(
		const float&                                 dt,
		const Weak<CommandPair>&                     w_cmd,
		const std::vector<Graphics::SBs::LightVPSB>& light_vps,
		const StrongScene&                           scene,
		const StrongLayer&                           lights
	)
	{
		// Second Pass: Intensity test
		// In light VP Render objects,
		// In pixel shader, while sampling shadow factor, do not use own shadow map
		// If shadow factor is above 0.f, and light intensity is above 0.f
		// then it intersects with light and shadow
		// Fill the pixel with light index (Do not use the sampler state, need raw value)
		constexpr size_t target_light_slot = 2;
		constexpr size_t custom_vp_slot    = 3;
		constexpr size_t custom_view_slot  = 1;
		constexpr size_t custom_proj_slot  = 2;

		const auto& cmd = w_cmd.lock();

		// Prepare the shadow depth texture as shader resource
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> shadow_dsv_srv;
		shadow_dsv_srv.reserve(g_max_lights);

		for (const auto& tex : m_shadow_texs_)
		{
			tex.ManualTransition(cmd->GetList(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
			shadow_dsv_srv.push_back(tex.GetSRVDescriptor()->GetCPUDescriptorHandleForHeapStart());
		}

		// Prepare the shadow mask texture (RTV result) as shader resource
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> shadow_rtv_srv;

		for (auto& tex : m_shadow_mask_texs_)
		{
			tex.ManualTransition(cmd->GetList(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
			shadow_rtv_srv.push_back(tex.GetSRVDescriptor()->GetCPUDescriptorHandleForHeapStart());
		}

		// Find the nearest clip space for z value.
		int z_clip = 0;

		if (const auto camera = scene->GetMainCamera().lock())
		{
			const float z = camera->GetComponent<Components::Transform>().lock()->GetWorldPosition().z;

			for (int i = 0; i < 3; ++i)
			{
				if (light_vps[0].end_clip_spaces[i].z > z)
				{
					z_clip = i;
					break;
				}
			}
		}

		for (int i = 0; i < lights->size(); ++i)
		{
			Graphics::SBs::LocalParamSB local_param{};
			local_param.SetParam<int>(target_light_slot, i);
			local_param.SetParam<int>(custom_vp_slot, true);
			local_param.SetParam<Matrix>(custom_view_slot, light_vps[i].view[z_clip]);
			local_param.SetParam<Matrix>(custom_proj_slot, light_vps[i].proj[z_clip]);
			auto& local_param_mem = m_second_pass_local_params_.get();
			local_param_mem.SetData(cmd->GetList(), 1, &local_param);
			m_second_pass_local_params_.advance();

			GetRenderer().RenderPass
					(
					 dt, SHADER_DOMAIN_OPAQUE, true,
					 cmd,
					 m_shadow_heaps_,
					 m_instance_buffers_,
					 [this](const StrongObjectBase& obj)
					 {
						 if (obj->GetID() == GetOwner().lock()->GetID())
						 {
							 return false;
						 }

						 return true;
					 }, [this, i, shadow_rtv_srv, shadow_dsv_srv](const Weak<CommandPair>& wc, const DescriptorPtr& wh)
					 {
						 const auto& c = wc.lock();
						 const auto& h = wh.lock();

						 c->GetList()->RSSetViewports(1, &m_viewport_);
						 c->GetList()->RSSetScissorRects(1, &m_scissor_rect_);
						 c->GetList()->SetPipelineState(m_intensity_test_shader_->GetPipelineState());
						 c->GetList()->IASetPrimitiveTopology(m_intensity_test_shader_->GetTopology());
						 h->SetSampler(m_intensity_test_shader_->GetShaderHeap(), 0);

						 GetShadowManager().BindShadowSampler(h);
						 GetRenderPipeline().BindConstantBuffers(c, h);

						 Resources::Texture* rtvs[]
						 {
							 &m_intensity_test_texs_[i],
							 &m_intensity_position_texs_[i]
						 };

						 Resources::Texture::Bind(c, rtvs, 2, *m_tmp_shadow_depth_);

						 h->SetShaderResources
								 (
								  BIND_SLOT_TEX,
								  shadow_rtv_srv.size(),
								  shadow_rtv_srv
								 );

						 h->SetShaderResources
								 (
								  RESERVED_TEX_SHADOW_MAP,
								  shadow_dsv_srv.size(),
								  shadow_dsv_srv
								 );
					 }, [this, i](const Weak<CommandPair>& wc, const DescriptorPtr& heap)
					 {
						 const auto& c = wc.lock();

						 Resources::Texture* rtvs[]
						 {
							 &m_intensity_test_texs_[i],
							 &m_intensity_position_texs_[i]
						 };

						 Resources::Texture::Unbind(c, rtvs, 2, *m_tmp_shadow_depth_);
					 },
					 {
						 &local_param_mem,
						 GetShadowManager().GetLightBuffer(),
						 GetShadowManager().GetLightVPBuffer()
					 }
					);

			m_tmp_shadow_depth_->Clear(cmd->GetList(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

			for (const auto& tex : m_shadow_texs_)
			{
				tex.ManualTransition
						(cmd->GetList(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
			}

			for (auto& tex : m_shadow_mask_texs_)
			{
				tex.ManualTransition
						(cmd->GetList(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
			}
		}
	}

	void ShadowIntersectionScript::ThirdPass(const Weak<CommandPair>& w_cmd, const StrongLayer& lights)
	{
		// Third Pass: Intersection Compute
		// By compute shader, For each pixel if it has light index, which is non-zero,
		// then this object shadow intersects with designated light.
		const auto& cmd = w_cmd.lock();

		m_shadow_third_pass_heap_ = GetRenderPipeline().AcquireHeapSlot().lock();

		for (int i = 0; i < lights->size(); ++i)
		{
			const auto& cast            = m_intersection_compute_->GetSharedPtr<ComputeShaders::IntersectionCompute>();
			auto&       local_param_mem = m_third_compute_local_param_.get();
			m_third_compute_local_param_.advance();

			cast->SetIntersectionTexture(m_intensity_test_texs_[i]);
			cast->SetPositionTexture(m_intensity_position_texs_[i]);
			cast->SetLightTable(m_sb_light_table_);
			cast->SetTargetLight(i);
			Graphics::SBs::LocalParamSB empty_param{};
			cast->Dispatch(cmd->GetList(), m_shadow_third_pass_heap_, empty_param, local_param_mem);
		}
	}

	void ShadowIntersectionScript::PostRender(const float& dt)
	{
		// Build shadow map of this object for each light.
		if (const auto& scene = GetOwner().lock()->GetScene().lock())
		{
			const auto& cmd = GetD3Device().AcquireCommandPair(L"Shadow Intersection Update").lock();

			cmd->SoftReset();

			for (auto& tex : m_shadow_texs_)
			{
				tex.Clear(cmd->GetList());
			}

			for (auto& tex : m_intensity_position_texs_)
			{
				tex.Clear(cmd->GetList(), D3D12_RESOURCE_STATE_RENDER_TARGET);
			}

			for (auto& tex : m_intensity_test_texs_)
			{
				tex.Clear(cmd->GetList(), D3D12_RESOURCE_STATE_RENDER_TARGET);
			}

			for (auto& tex : m_shadow_mask_texs_)
			{
				tex.Clear(cmd->GetList(), D3D12_RESOURCE_STATE_RENDER_TARGET);
			}

			std::vector<Graphics::SBs::LightVPSB> light_vps;
			constexpr size_t                      shadow_slot = 0;
			const auto                            lights      = (*scene)[LAYER_LIGHT];

			// Pre-allocate the light table
			static std::vector<ComputeShaders::IntersectionCompute::LightTableSB> empty_light_table;
			if (empty_light_table.size() < lights->size())
			{
				empty_light_table.resize(lights->size());
			}

			// Cleanup the light table
			m_sb_light_table_->SetData(cmd->GetList(), lights->size(), empty_light_table.data());

			constexpr size_t render_pass = 3;

			m_instance_buffers_.resize(GetRenderer().GetInstanceCount() * (lights->size() * render_pass) + 1);
			m_first_other_pass_local_params_.resize
					(GetRenderer().GetInstanceCount() * (lights->size() * render_pass) + 1);
			m_first_self_pass_local_params_.resize
					(GetRenderer().GetInstanceCount() * (lights->size() * render_pass) + 1);
			m_second_pass_local_params_.resize(lights->size());
			m_third_compute_local_param_.resize(lights->size());

			m_instance_buffers_.reset();
			m_first_other_pass_local_params_.reset();
			m_first_self_pass_local_params_.reset();
			m_second_pass_local_params_.reset();
			m_third_compute_local_param_.reset();

			GetShadowManager().GetLightVP(scene, light_vps);

			FirstPass(dt, cmd, shadow_slot, lights);
			SecondPass(dt, cmd, light_vps, scene, lights);
			ThirdPass(cmd, lights);

			cmd->Execute(true);

			std::vector<ComputeShaders::IntersectionCompute::LightTableSB> light_table;
			light_table.resize(lights->size());
			m_sb_light_table_->GetData(lights->size(), light_table.data());

			// Build bounding box data from result of compute shader.
			for (int i = 0; i < lights->size(); ++i)
			{
				for (int j = 0; j < lights->size(); ++j)
				{
					if (light_table[i].lightTable[j].value > 0)
					{
						const auto wp_min = Vector3(light_table[i].min[j]) / light_table[i].min[j].w;
						const auto wp_max = Vector3(light_table[i].max[j]) / light_table[i].max[j].w;

						const auto average = Vector3::Lerp(wp_min, wp_max, 0.5f);

						GetDebugger().Draw(BoundingSphere(average, 0.1f), Colors::YellowGreen);

						BoundingBox bbox;
						BoundingBox::CreateFromPoints
								(
								 bbox,
								 wp_min,
								 wp_max
								);

						m_shadow_bbox_.emplace(std::make_pair(i, j), bbox);
					}
				}
			}
		}
	}

	void ShadowIntersectionScript::OnCollisionEnter(const WeakCollider& other) {}

	void ShadowIntersectionScript::OnCollisionContinue(const WeakCollider& other) {}

	void ShadowIntersectionScript::OnCollisionExit(const WeakCollider& other) {}

	ShadowIntersectionScript::ShadowIntersectionScript()
		: m_viewport_(),
		  m_scissor_rect_() {}
}
