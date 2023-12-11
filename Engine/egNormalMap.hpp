#pragma once
#include "egTexture.hpp"

namespace Engine::Resources
{
	class NormalMap : public Texture
	{
	public:
		explicit NormalMap(std::filesystem::path path) : Texture(std::move(path))
		{}

		~NormalMap() override = default;

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		TypeName GetVirtualTypeName() const final;

		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;
	protected:
		NormalMap() : Texture({}){}

	private:
		SERIALIZER_ACCESS

	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::NormalMap)