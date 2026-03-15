#pragma once
#include <filesystem>

#include "TypeLibrary.h"
#include "Entity.h"

#include "Resource.generated.h"

namespace Engine
{
	using ResourceType = HashType;
}

namespace Engine::Abstracts
{
	ECLASS(abstract, serialize)
	class ENGINE_CORE_API Resource : public Entity
	{
	public:
		GENERATE_BODY

		~Resource() override;
		Resource(const Resource& other);
		Resource& operator=(const Resource& other);

		virtual void Load() final;
		void         Unload();

#if WITH_EDITOR
		void OnUIUpdate(UIContext* const parent, const float dt) override;
#endif
		void OnDeserialized() override;

		[[nodiscard]] bool                         IsLoaded() const;
		[[nodiscard]] const std::filesystem::path& GetPath() const;
		[[nodiscard]] Strong<Resource> Clone() const;
		
		void SetPath(const std::filesystem::path& path);

	protected:
		Resource(std::filesystem::path path);

		virtual Strong<Resource> cloneImpl() const = 0;
		virtual void Load_INTERNAL() = 0;
		virtual void Unload_INTERNAL() = 0;

	private:
		Resource();
		friend class Managers::ResourceManager;

		bool                    m_bLoaded_;

		EPROPERTY()
		std::filesystem::path m_path_;
	};
} // namespace Engine::Abstract

namespace Engine
{
	template struct ENGINE_CORE_API FactoryTemplate<Engine::Abstracts::Resource, const std::filesystem::path&>;
	using ResourceFactory = FactoryTemplate<Engine::Abstracts::Resource, const std::filesystem::path&>;
}

#define REGISTER_RESOURCE(TYPE) Engine::ResourceFactory::Register<##TYPE##>();
#define UNREGISTER_RESOURCE(TYPE) Engine::ResourceFactory::Unregister<##TYPE##>();