#pragma once
#include <filesystem>

#include "TypeLibrary.h"
#include "Entity.h"

#include "Resource.generated.h"

// Static resource getter which infers self as type
#define RESOURCE_SELF_INFER_GETTER(TYPE) \
	template <typename Void = void> requires (std::is_base_of_v<Engine::Abstracts::Resource, TYPE##>)\
	static Engine::Weak<TYPE> Get(const std::string& name) { return Engine::Managers::ResourceManager::GetInstance().GetResource<TYPE>(name); }\
	template <typename Void = void> requires (std::is_base_of_v<Engine::Abstracts::Resource, TYPE##>)\
	static Engine::Weak<TYPE> GetByMetadataPath(const std::filesystem::path& meta_path) { return Engine::Managers::ResourceManager::GetInstance().GetResourceByMetadataPath<TYPE>(meta_path); } \
	template <typename Void = void> requires (std::is_base_of_v<Engine::Abstracts::Resource, TYPE##>)\
	static Engine::Weak<TYPE> GetByRawPath(const std::filesystem::path& path) { return Engine::Managers::ResourceManager::GetInstance().GetResourceByRawPath<TYPE>(path); }

// Static resource create function
// using std::enable_if_t<...> in template throws error of undefined class, however requires keyword works fine.
#define RESOURCE_SELF_INFER_CREATE(TYPE)\
template <typename... Args> requires (std::is_base_of_v<Engine::Abstracts::Resource, TYPE##>)\
static Engine::Strong<TYPE> Create(const std::string_view name, Args&&... args)\
{\
if (!name.empty() && Engine::Managers::ResourceManager::GetInstance().GetResource<##TYPE##>(name).lock()) { return {}; }\
const auto obj = boost::make_shared<##TYPE##>(std::forward<Args>(args)...);\
Engine::Managers::ResourceManager::GetInstance().AddResource(name, obj);\
return obj;\
}

// Cloning resource declaration macro
#define RES_CLONE_DECL Engine::Strong<Engine::Abstracts::Resource> cloneImpl() const override;
// Cloning resource implementation macro
#define RES_CLONE_IMPL(CLASS) Engine::Strong<Engine::Abstracts::Resource> CLASS::cloneImpl() const { return make_managed_shared<CLASS>(*this); }

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