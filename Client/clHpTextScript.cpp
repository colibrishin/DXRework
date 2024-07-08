#include "pch.h"
#include "clHpTextScript.h"

#include "egCamera.h"
#include "egSceneManager.hpp"
#include "egText.h"

SERIALIZE_IMPL(Client::Scripts::HpTextScript, _ARTAG(_BSTSUPER(Script)))

namespace Client::Scripts
{
	SCRIPT_CLONE_IMPL(HpTextScript)

	HpTextScript::HpTextScript() = default;

	void HpTextScript::Initialize()
	{
		Script::Initialize();

		const auto& obj   = GetOwner().lock();
		const auto  child = obj->GetScene().lock()->CreateGameObject<Objects::Text>
				(LAYER_UI, Resources::Font::Get("DefaultFont")).lock();

		child->SetText("");
		child->SetPosition({0.f, 64.f});
		child->SetColor({1.f, 1.f, 1.f, 1.f});
		child->SetScale(1.f);
	}

	void HpTextScript::PreUpdate(const float& dt) {}

	void HpTextScript::Update(const float& dt)
	{
		if (const auto hp_text = GetOwner().lock()->GetSharedPtr<Objects::Text>())
		{
			if (const auto player = hp_text->GetParent().lock())
			{
				if (const auto cc = player->GetScript<PlayerScript>().lock())
				{
					const auto hp = cc->GetHealth();
					hp_text->SetText(std::to_string(hp));
				}
			}
		}
	}

	void HpTextScript::PostUpdate(const float& dt) {}

	void HpTextScript::FixedUpdate(const float& dt) {}

	void HpTextScript::PreRender(const float& dt) {}

	void HpTextScript::Render(const float& dt)
	{
		if (const auto hp_text = GetOwner().lock()->GetSharedPtr<Objects::Text>())
		{
			const auto parent = hp_text->GetParent().lock();

			const auto scene = GetSceneManager().GetActiveScene().lock();
			const auto cam   = scene->GetMainCamera().lock();
			const auto head  = cam->GetParent().lock();

			if (head && head->GetParent().lock() != parent)
			{
				hp_text->SetActive(false);
			}
			else
			{
				hp_text->SetActive(true);
			}
		}
	}

	void HpTextScript::PostRender(const float& dt) {}

	void HpTextScript::OnCollisionEnter(const WeakCollider& other) {}

	void HpTextScript::OnCollisionContinue(const WeakCollider& other) {}

	void HpTextScript::OnCollisionExit(const WeakCollider& other) {}
}
