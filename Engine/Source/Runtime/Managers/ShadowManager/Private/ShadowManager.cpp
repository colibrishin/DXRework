#include "ShadowManager.h"
#include "ShadowManager.generated.h"

#include "Source/Runtime/Core/Components/Transform/Public/Transform.h"
#include "Source/Runtime/Core/Objects/Camera/Public/Camera.h"
#include "Source/Runtime/Core/Objects/Light/Public/Light.h"
#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.h"
#include "Source/Runtime/Core/Scene/Public/Scene.h"
#include "Source/Runtime/Core/SceneManager//Public/SceneManager.h"

#include "RenderPipeline.h"
#include "Renderer.h"
#include "Shader.h"
#include "ShadowRenderTarget.h"
#include "ShadowTexture.h"
#include "ForwardRenderPassTask.h"

namespace Engine::Managers
{
	void ShadowManager::Initialize()
	{
		m_shadow_shader_ = Resources::Shader::Create(
				 "cascade_shadow_stage1", "./cascade_shadow_stage1.hlsl", 
				 SHADER_DOMAIN_OPAQUE, true, SHADER_DEPTH_TEST_ALL, SHADER_DEPTH_LESS_EQUAL,
				 SHADER_SAMPLER_CLAMP, SHADER_SAMPLER_LESS_EQUAL,
				 SAMPLER_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
				 SHADER_RASTERIZER_CULL_BACK, SHADER_RASTERIZER_FILL_SOLID,
				 GetDefaultRTVFormat(), TEX_FORMAT_D32_FLOAT,
				 PRIMITIVE_TOPOLOGY_TRIANGLELIST, PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
				 SAMPLER_SHADOW);

		GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();
		m_light_sb_ = std::make_unique<decltype(m_light_sb_)::element_type>(gi.GetStructuredBuffer<SBs::LightSB>());
		m_light_vp_sb_ = std::make_unique<decltype(m_light_vp_sb_)::element_type>(gi.GetStructuredBuffer<SBs::LightVPSB>());
        
		InitializeViewport();

		SceneManager::GetInstance().onSceneRemoved.Listen(GetSharedPtr<ShadowManager>(), &ShadowManager::PreSwapScene);
		SceneManager::GetInstance().onSceneActive.Listen(GetSharedPtr<ShadowManager>(), &ShadowManager::PostSwapScene);

		if (const Strong<Scene>& scene = SceneManager::GetInstance().GetActiveScene().lock())
		{
			PostSwapScene(scene);

			for (const Weak<Abstracts::ObjectBase>& object : scene->GetGameObjects(RESERVED_LAYER_LIGHT))
			{
				if (const Strong<Objects::Light>& locked = Cast<Objects::Light>(object))
				{
					RegisterLight(locked->GetSharedPtr<Objects::Light>());
				}
			}
		}

		m_shadow_sampler_ = Unique<decltype( m_shadow_sampler_ )::element_type>( gi.GetNewPrimitiveSampler() );
        m_shadow_sampler_->Generate(
                SHADER_SAMPLER_WRAP, 
				SHADER_SAMPLER_LESS_EQUAL, 
				SAMPLER_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT );

		Renderer::GetInstance().RegisterStructuredBuffer(m_light_sb_.get());
		Renderer::GetInstance().RegisterStructuredBuffer(m_light_vp_sb_.get());

		Renderer::GetInstance().RegisterContextPreRenderSetup("Shadow Manager", [](const GraphicInterfaceContextPrimitive* context)
		{
			GetInstance().BindShadowMaps(context);
		});
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
        current_light_vp.resize( m_lights_.size() );
		size_t idx = 0;

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
					current_light_vp.at(idx) = {};
					continue;
				}

				SBs::LightVPSB light_vp{};
				// Get the light's view and projection matrix in g_max_shadow_cascades parts.
				EvalShadowVP(scene->GetMainCamera(), light_dir, light_vp);
				current_light_vp.at(idx) = light_vp;
			}
			++idx;
		}
	}

	void ShadowManager::PreRender(const float dt)
	{
		constexpr size_t light_slot = 0;

		// # Pass 1 : depth only, building shadow map

		// Notify the number of lights to the shader.
		RenderPipeline::GetInstance().SetParam<int>(static_cast<UINT>(m_lights_.size()), light_slot);

		// If there is no light, it does not need to be updated.
		if (m_lights_.empty())
		{
			return;
		}

		if (const auto scene = SceneManager::GetInstance().GetActiveScene().lock())
		{
			GetLightVP(scene, m_current_scene_light_vp_);

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

			GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();
			
			{
				const auto& context = gi.GetNewContext(0, false, L"Light Structured Buffer Transition");
				const auto& primitive = context.GetPointers();

				primitive.commandList->SoftReset();
				ClearShadowMaps(&primitive);
				CheckSize<UINT>(light_buffer.size(), L"Warning: Light buffer size is too big!");
				CheckSize<UINT>(m_current_scene_light_vp_.size(), L"Warning: Light VP size is too big!");

				m_light_sb_->SetData(&primitive, m_lights_.size(), light_buffer.data());
				m_light_vp_sb_->SetData(&primitive, m_current_scene_light_vp_.size(), m_current_scene_light_vp_.data());
				primitive.commandList->FlagReady();
			}
			
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

	void ShadowManager::BuildShadowMap(const float dt, const Strong<Objects::Light>& light, const UINT light_idx) const
	{
		// Notify the light index to the shader.
		SBs::LocalParamSB local_param{};
		local_param.SetParam(0, static_cast<int>(light_idx));

		GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();
		{
			const auto context   = gi.GetNewContext(0, false, L"Shadow Map Transition");
			const auto primitive = context.GetPointers();
			primitive.commandList->SoftReset();
			gi.TransitTo(&primitive, m_shadow_texs_.at(light->GetLocalID()).get(), BIND_TYPE_DSV);
			primitive.commandList->FlagReady();
		}

		Renderer::GetInstance().RenderPassVanillaInclusion<SHADER_DOMAIN_OPAQUE, Engine::ForwardRenderPassTask>
			(
			 dt, true, local_param, { m_light_sb_.get(), m_light_vp_sb_.get() },
			 [](const Strong<Abstracts::ObjectBase>& obj)
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
			     gi.SetViewport( context, m_viewport_ );
				 gi.BindGraphic(context, m_shadow_shader_.get());
				 gi.BindMultiple(context, nullptr, 0, m_shadow_texs_.at(light->GetLocalID()).get());
			 }, {}, {}, {}
			);

		{
			const auto context   = gi.GetNewContext(0, false, L"Shadow Map Transition To Common");
			const auto primitive = context.GetPointers();
			primitive.commandList->SoftReset();
			gi.TransitBack(&primitive, m_shadow_texs_.at(light->GetLocalID()).get(), BIND_TYPE_DSV);
			gi.TransitTo(&primitive, m_shadow_texs_.at(light->GetLocalID()).get(), BIND_TYPE_SRV);
			primitive.commandList->FlagReady();
		}
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
			constexpr float near_plane = CFG_SCREEN_NEAR;
			constexpr float far_plane  = CFG_SCREEN_FAR;

			// todo: evaluate in log scale
			constexpr float cascadeEnds[]{near_plane, 10.f, 80.f, far_plane};

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

		context->heap->SetSampler( m_shadow_sampler_.get(), SAMPLER_SHADOW );
	}

	void ShadowManager::TransitBackShadowMaps(const GraphicInterfaceContextPrimitive* context) const
	{
		GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();
		aligned_vector<Resources::Texture*> textures;

		for (const auto& tex : m_shadow_texs_ | std::views::values)
		{
			textures.push_back(tex.get());
		}

		CheckSize<UINT>(textures.size(), L"Warning: Shadow map size is too big!");
		gi.TransitBackMultiple(context, textures.data(), textures.size(), BIND_TYPE_SRV);
	}

    StructuredBufferTypeProxy<SBs::LightSB> & ShadowManager::GetLightBuffer() const
    {
	    return *m_light_sb_;
	}

    StructuredBufferTypeProxy<SBs::LightVPSB> & ShadowManager::GetLightVPBuffer() const
    {
	    return *m_light_vp_sb_;
	}

    const std::vector<SBs::LightVPSB> & ShadowManager::GetCurrentSceneLightVP() const
	{
	    return m_current_scene_light_vp_;
	}

    void ShadowManager::RegisterLight(Weak<Abstracts::ObjectBase> light)
	{
		if (const auto locked = light.lock())
		{
			if (locked->IsDerivedOf(Objects::Light::StaticTypeHash()))
			{
				m_lights_[locked->GetLocalID()] = locked->GetSharedPtr<Objects::Light>();
				InitializeShadowBuffer(locked->GetLocalID());
			}
		}
	}

	void ShadowManager::UnregisterLight(Weak<Abstracts::ObjectBase> light)
	{
		if (const auto locked = light.lock())
		{
			if (locked->IsDerivedOf(Objects::Light::StaticTypeHash()))
			{
				m_lights_.erase(locked->GetLocalID());
				m_shadow_texs_.erase(locked->GetLocalID()); // todo: remove from resource manager
			}
		}
	}

	void ShadowManager::InitializeShadowBuffer(const LocalActorID id)
	{
		m_shadow_texs_[id] = Resources::ShadowTexture::Create("Shadow texture " + std::to_string(id));
		m_shadow_texs_[id]->Load();
	}

	ShadowManager::~ShadowManager()
	{
		Renderer::GetInstance().UnregisterStructuredBuffer(m_light_sb_.get());
		Renderer::GetInstance().UnregisterStructuredBuffer(m_light_vp_sb_.get());
		
		Renderer::GetInstance().UnregisterContextPreRenderSetup("Shadow Manager");
	}

	void ShadowManager::PreSwapScene(Weak<Scene> scene)
	{
		if (const Strong<Scene>& locked = scene.lock())
		{
			locked->onObjectAdded.Remove(GetSharedPtr<ShadowManager>(), &ShadowManager::RegisterLight);
			locked->onObjectRemoved.Remove(GetSharedPtr<ShadowManager>(), &ShadowManager::UnregisterLight);
		}
	}

	void ShadowManager::PostSwapScene(Weak<Scene> scene)
	{
		if (const Strong<Scene>& locked = scene.lock())
		{
			locked->onObjectAdded.Listen(GetSharedPtr<ShadowManager>(), &ShadowManager::RegisterLight);
			locked->onObjectRemoved.Listen(GetSharedPtr<ShadowManager>(), &ShadowManager::UnregisterLight);
		}
	}

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
		GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();
		
		for (auto& tex : m_shadow_texs_ | std::views::values)
		{
			gi.TransitBack(context, tex.get(), BIND_TYPE_SRV);
			tex->Clear(context);
		}
	}
} // namespace Engine::Managers