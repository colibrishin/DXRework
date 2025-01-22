#include "../Public/ShadowManager.h"

#include "Source/Runtime/Core/Components/Transform/Public/Transform.h"
#include "Source/Runtime/Core/Objects/Camera/Public/Camera.h"
#include "Source/Runtime/Core/Objects/Light/Public/Light.h"
#include "Source/Runtime/Core/Scene/Public/Scene.h"

#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"
#include "Source/Runtime/Resources/Shader/Public/Shader.h"
#include "Source/Runtime/Resources/ShadowTexture/Public/ShadowTexture.h"

#include "Source/Runtime/Managers/RenderPipeline/Public/Renderer.h"
#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.h"
#include "Source/Runtime/Core/SceneManager/Public/SceneManager.h"

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
				 "Shadow Render Target Texture",
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

		GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();
		m_light_sb_ = gi.GetStructuredBuffer<SBs::LightSB>();
		m_light_vp_sb_ = gi.GetStructuredBuffer<SBs::LightVPSB>();

		InitializeViewport();
	}

	void ShadowManager::PreUpdate(const float dt)
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

	void ShadowManager::Update(const float dt) {}

	void ShadowManager::GetLightVP(const Strong<Scene>& scene, std::vector<SBs::LightVPSB>& current_light_vp)
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

	void ShadowManager::PreRender(const float dt)
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

		if (const auto scene = SceneManager::GetInstance().GetActiveScene().lock())
		{
			GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();
			const GraphicInterfaceContextReturnType& context = gi.GetNewContext(0, false, L"Depth pass for Shadow");
			const GraphicInterfaceContextPrimitive& primitive = context.GetPointers();

			primitive.commandList->SoftReset();
			std::vector<SBs::LightVPSB> current_light_vp;
			GetLightVP(scene, current_light_vp);

			// Also, if there is no light, it does not need to be updated.
			if (current_light_vp.empty())
			{
				return;
			}

			ClearShadowMaps(&primitive);
			CheckSize<UINT>(light_buffer.size(), L"Warning: Light buffer size is too big!");
			CheckSize<UINT>(current_light_vp.size(), L"Warning: Light VP size is too big!");

			m_light_sb_.GetTypeless().TransitionCommon(&primitive);
			m_light_vp_sb_.GetTypeless().TransitionCommon(&primitive);

			m_light_sb_.SetData(&primitive, static_cast<UINT>(light_buffer.size()), light_buffer.data());
			m_light_vp_sb_.SetData(&primitive, static_cast<UINT>(current_light_vp.size()), current_light_vp.data());

			m_light_sb_.GetTypeless().TransitionToSRV(&primitive);
			m_light_vp_sb_.GetTypeless().TransitionToSRV(&primitive);
			primitive.commandList->FlagReady();
			
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

	void ShadowManager::Render(const float dt) {}

	void ShadowManager::PostRender(const float dt) {}

	void ShadowManager::FixedUpdate(const float dt) {}

	void ShadowManager::PostUpdate(const float dt) {}

	void ShadowManager::Reset()
	{
		m_lights_.clear();
		m_shadow_texs_.clear();

		for (auto& subfrusta : m_subfrusta_)
		{
			subfrusta = {};
		}
	}

	void ShadowManager::BuildShadowMap(const float dt, const Strong<Objects::Light>& light, const UINT light_idx)
	{
		// Notify the light index to the shader.
		SBs::LocalParamSB local_param{};
		local_param.SetParam(0, static_cast<int>(light_idx));

		GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();
		Renderer::GetInstance().RenderPass
			(
			 dt, true, SHADER_DOMAIN_OPAQUE, local_param, [](const Strong<Abstracts::ObjectBase>& obj)
			 {
				 if (obj->GetLayer() == RESERVED_LAYER_CAMERA ||
				     obj->GetLayer() == RESERVED_LAYER_UI ||
				     obj->GetLayer() == RESERVED_LAYER_ENVIRONMENT ||
				     obj->GetLayer() == RESERVED_LAYER_LIGHT ||
				     obj->GetLayer() == RESERVED_LAYER_SKYBOX) { return false; }

				 return true;
			 },
			 [&gi, this, &light](const GraphicInterfaceContextPrimitive* context)
			 {
				 gi.BindGraphic(context, m_shadow_shader_.get());
				 Resources::Texture* temp_tex_arr[] = {m_shadow_map_mask_.get()};
				 gi.BindMultiple(context, temp_tex_arr, 1, m_shadow_texs_.at(light->GetLocalID()).get());
				 BindShadowMaps(context);
			 },
			 [this, &gi, &light](const GraphicInterfaceContextPrimitive* context)
			 {
				 Resources::Texture* temp_tex_arr[] = {m_shadow_map_mask_.get()};
				 gi.UnbindMultiple(context, temp_tex_arr, 1, m_shadow_texs_.at(light->GetLocalID()).get());
				 UnbindShadowMaps(context);
			 }
			);
	}

	void ShadowManager::CreateSubfrusta(
		const Matrix& projection, float start,
		float         end, Subfrusta&   subfrusta
	)
	{
		BoundingFrustum frustum(projection);

		frustum.Near = start;
		frustum.Far  = end;

		static constexpr DirectX::XMVECTORU32 vGrabY = {
			0x00000000, 0xFFFFFFFF, 0x00000000,
			0x00000000
		};
		static constexpr DirectX::XMVECTORU32 vGrabX = {
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
					DirectX::XMMatrixTranspose
						(
						 DirectX::XMMatrixOrthographicOffCenterLH
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

	void ShadowManager::BindShadowMaps(const GraphicInterfaceContextPrimitive* context) const
	{
		GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();
		aligned_vector<Resources::Texture*> textures;

		for (const auto& tex : m_shadow_texs_ | std::views::values)
		{
			textures.push_back(tex.get());
		}

		CheckSize<UINT>(textures.size(), L"Warning: Shadow map size is too big!");
		gi.BindMultiple(context, textures.data(), BIND_TYPE_SRV, RESERVED_TEX_SHADOW_MAP, 0, textures.size());
	}

	void ShadowManager::UnbindShadowMaps(const GraphicInterfaceContextPrimitive* context) const
	{
		GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();
		aligned_vector<Resources::Texture*> textures;

		for (const auto& tex : m_shadow_texs_ | std::views::values)
		{
			textures.push_back(tex.get());
		}

		CheckSize<UINT>(textures.size(), L"Warning: Shadow map size is too big!");
		gi.UnbindMultiple(context, textures.data(), BIND_TYPE_SRV, textures.size());
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
		m_shadow_texs_[id] = Resources::ShadowTexture::Create("Shadow texture", "");
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
	}

	void ShadowManager::ClearShadowMaps(const GraphicInterfaceContextPrimitive* context)
	{
		for (auto& tex : m_shadow_texs_ | std::views::values)
		{
			tex->Clear(context);
		}

		GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();
		gi.Clear(context, m_shadow_map_mask_.get(), BIND_TYPE_RTV);
	}
} // namespace Engine::Managers