#pragma once
#include "egScene.hpp"

namespace Engine
{
	class EditorScene : public Scene
	{
	public:
		~EditorScene() override = default;

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void FixedUpdate(const float& dt) override;

	private:
		void OnDeserialized() override;

		void EditorScreen();
	};
}
