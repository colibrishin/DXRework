#pragma once
#include "Entity.h"
#include "ObjectBase.h"
#include "ConstantBuffer.h"

#include "Camera.generated.h"

namespace Engine::Objects
{
	ECLASS(serialize)
	class ENGINE_CORE_API Camera final : public Abstracts::ObjectBase
	{
		GENERATE_BODY
	public:
		OBJECT_T(DEF_OBJ_T_CAMERA)

		Camera()
			: ObjectBase(DEF_OBJ_T_CAMERA),
			  m_fov_(20.0f),
			  m_b_orthogonal_(false),
			  m_b_fixed_up_(true) {}

		~Camera() override = default;

		void Initialize() override;
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void PostRender(const float dt) override;
		void FixedUpdate(const float dt) override;
		void OnDeserialized() override;

		void SetOrthogonal(bool bOrthogonal);
		void SetFixedUp(bool bFixedUp);
		void SetFOV(float zoom);

		[[nodiscard]] Matrix  GetViewMatrix() const;
		[[nodiscard]] Matrix  GetProjectionMatrix() const;
		[[nodiscard]] Matrix  GetWorldMatrix() const;
		[[nodiscard]] bool    GetOrthogonal() const;
		[[nodiscard]] float   GetFOV() const;
		[[nodiscard]] const Graphics::CBs::PerspectiveCB& GetPerspectiveCB() const;

	private:
		OBJ_CLONE_DECL;

	private:
		friend class Managers::CameraManager; 

		EPROPERTY()
		float m_fov_;

		EPROPERTY()
		bool  m_b_orthogonal_;

		EPROPERTY()
		bool  m_b_fixed_up_;

		// Non-serialized
		Matrix m_world_matrix_;
		Matrix m_view_matrix_;
		Matrix m_projection_matrix_;

		Graphics::CBs::PerspectiveCB m_perspective_cb_;
	};
} // namespace Engine::Objects
