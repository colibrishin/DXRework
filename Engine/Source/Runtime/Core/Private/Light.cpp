#include "Light.h"

#include <magic_enum/magic_enum.hpp>

#include "Light.generated.h"

#include "Transform.h"

namespace Engine::Objects
{
	OBJ_CLONE_IMPL(Light)

	Light::Light()
		: ObjectBase(DEF_OBJ_T_LIGHT),
		  m_radius_(0.5f),
		  m_range_(10.f),
		  m_type_(LIGHT_T_DIRECTIONAL) {}

	Light::~Light() {}

	void Light::SetColor(Vector4 color)
	{
		m_color_ = color;
	}

	void Light::SetType(eLightType type)
	{
		m_type_ = type;

		if (m_type_ == LIGHT_T_DIRECTIONAL)
		{
			SetRange(0.0f);
		}
		else
		{
			SetRange(10.0f);
		}
	}

	void Light::SetRange(float range)
	{
		if (m_type_ == LIGHT_T_DIRECTIONAL)
		{
			return;
		}

		m_range_ = range;
	}

	void Light::Initialize()
	{
		ObjectBase::Initialize();
		AddComponent<Components::Transform>();
		m_color_ = Vector4{1.0f, 1.0f, 1.0f, 1.0f};
		SetCulled(false);
	}

	void Light::PreUpdate(const float dt)
	{
		ObjectBase::PreUpdate(dt);
	}

	void Light::Update(const float dt)
	{
		ObjectBase::Update(dt);
	}

	void Light::PreRender(const float dt)
	{
		ObjectBase::PreRender(dt);
	}

	void Light::Render(const float dt)
	{
		ObjectBase::Render(dt);
	}

	void Light::PostRender(const float dt)
	{
		ObjectBase::PostRender(dt);
	}

	void Light::PostUpdate(const float dt)
	{
		ObjectBase::PostUpdate(dt);
    }

#if WITH_EDITOR
    void Light::OnUIUpdate( UIContext *const parent, const float dt )
    {
        if ( parent )
        {
            ObjectBase::OnUIUpdate( parent, dt );

            if ( m_ui_info_.dialogOpened )
            {
                IUIAPI &         ui              = g_ui_accessor.GetInterface();
                static constexpr auto light_type_enum = CStrEnumStrings<eLightType>();

                if ( const UIContext context = IUIAPI::NewContext(
                        ui.NewDialog( this, "LightDialog", { m_ui_info_.label, m_ui_info_.dialogOpened } ) ) )
                {
                    context |= ui.NewLabelAndFloat( this, "Radius", { "Radius", m_radius_, 0.f, 0.f, FLT_MAX, true } );
                    context |= ui.NewLabelAndFloat( this, "Range", { "Range", m_range_, 0.f, 0.f, FLT_MAX, true } );
                    context |= ui.NewCombobox( this,
                                               "LightType",
                                               { "Light type",
                                                 reinterpret_cast<int *>( &m_type_ ),
                                                 light_type_enum.data(),
                                                 light_type_enum.size(),
                                                 true } );
                    context |= ui.NewLabelAndVec4( this, "LightColor", { "Color", &m_color_.x, 0.f, 0.f, 1.f, true } );
                }
            }
        }
    }
#endif

	void Light::OnDeserialized()
	{
		ObjectBase::OnDeserialized();
	}
} // namespace Engine::Objects
