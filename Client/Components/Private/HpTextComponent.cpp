#include "Components/Public/HpTextComponent.h"
#include "HpTextComponent.generated.h"

#include "ObjectBase/Public/ObjectBase.h"
#include "Scene/Public/Scene.h"

void HpTextComponent::Initialize()
{
	Component::Initialize();
	SetText("");
	SetPosition({ 0.f, 64.f });
	SetColor({ 1.f, 1.f, 1.f, 1.f });
	SetScale({ 1.f, 1.f });
}