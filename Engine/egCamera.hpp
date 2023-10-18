#pragma once
#include <memory>
#include <SimpleMath.h>

#include "egCommon.hpp"
#include "egObject.hpp"

namespace Engine::Objects
{
	class Camera final : public Abstract::Object
	{
	public:
		Camera() = default;
		~Camera() override = default;

		void SetLookAt(Vector3 lookAt);
		void SetPosition(Vector3 position);
		void SetRotation(Quaternion rotation);

		void GetViewMatrix(DirectX::XMMATRIX& viewMatrix);

		void Initialize() override;
		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Render() override;

	private:
		Matrix m_view_matrix_;
		VPBuffer m_vp_buffer_;

		Vector3 m_look_at_;
		Quaternion m_rotation_;
	};
}
