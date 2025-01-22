#pragma once
#include <bitset>
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.hpp"

namespace Engine 
{
	enum CORE_API eLightType
	{
		LIGHT_T_UNK = 0,
		LIGHT_T_DIRECTIONAL,
		LIGHT_T_SPOT,
	};
}

namespace Engine::Objects
{
	class CORE_API Light final : public Abstracts::ObjectBase
	{
	public:
		OBJECT_T(DEF_OBJ_T_LIGHT)

		Light();
		~Light() override;

		void SetColor(Vector4 color);
		void SetType(eLightType type);
		void SetRange(float range);

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;
		void PostUpdate(const float& dt) override;

		void OnDeserialized() override;

		Color GetColor() const
		{
			return m_color_;
		}

		eLightType GetType() const
		{
			return m_type_;
		}

		float GetRange() const
		{
			return m_range_;
		}

		float GetRadius() const
		{
			return m_radius_;
		}

	private:
		OBJ_CLONE_DECL

		float      m_radius_;
		float      m_range_;
		eLightType m_type_;
		Color      m_color_;
	};
} // namespace Engine::Objects
