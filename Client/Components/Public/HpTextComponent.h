#pragma once
#include "TextRenderer.h"

#include "HpTextComponent.generated.h"

ECLASS(component=client, serialize)
class ENGINE_CLIENT_API HpTextComponent : public Engine::Components::TextRenderer
{
	GENERATE_BODY
public:
	void Initialize() override;

private:
	using TextRenderer::TextRenderer;
	HpTextComponent() : TextRenderer({}) {};
};