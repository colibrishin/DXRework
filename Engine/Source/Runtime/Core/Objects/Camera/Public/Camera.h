#pragma once
#include "Source/Runtime/Core/Entity/Public/Entity.hpp"
#include "Source/Runtime/Core/Objects/Object/Public/Object.hpp"
#include "Source/Runtime/ConstantBufferDX12/Public/ConstantBuffer.hpp"

namespace Engine::Objects
{
	class Camera final : public Abstracts::ObjectBase
	{
	public:
		OBJECT_T(DEF_OBJ_T_CAMERA)

		Camera()
			: ObjectBase(DEF_OBJ_T_CAMERA),
			  m_fov_(20.0f),
			  m_b_orthogonal_(false),
			  m_b_fixed_up_(true) {}

		~Camera() override = default;

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void OnDeserialized() override;

		void SetOrthogonal(bool bOrthogonal);
		void SetFixedUp(bool bFixedUp);
		void SetFOV(float zoom);

		Matrix  GetViewMatrix() const;
		Matrix  GetProjectionMatrix() const;
		Matrix  GetWorldMatrix() const;
		Vector2 GetWorldMousePosition();
		bool    GetOrthogonal() const;
		float   GetFOV() const;

	private:
		SERIALIZE_DECL
		OBJ_CLONE_DECL

	private:
		float m_fov_;
		bool  m_b_orthogonal_;
		bool  m_b_fixed_up_;

		// Non-serialized
		Matrix m_world_matrix_;
		Matrix m_view_matrix_;
		Matrix m_projection_matrix_;

		Graphics::CBs::PerspectiveCB m_wvp_buffer_;
	};
} // namespace Engine::Objects

BOOST_CLASS_EXPORT_KEY(Engine::Objects::Camera)
