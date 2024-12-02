#pragma once
#include "Source/Runtime/Abstracts/CoreEntity/Public/Entity.hpp"

// Static Resource type, this should be added to every resource
#define RESOURCE_T(enum_val) static constexpr eResourceType rtype = enum_val;

// Static resource getter which infers self as type
#define RESOURCE_SELF_INFER_GETTER_DECL(TYPE)                                         \
  static boost::weak_ptr<TYPE> Get(const std::string& name);                          \
  static boost::weak_ptr<TYPE> GetByMetadataPath(const std::filesystem::path& path);  \
  static boost::weak_ptr<TYPE> GetByRawPath(const std::filesystem::path& path);

#define RESOURCE_SELF_INFER_GETTER_IMPL(TYPE)										  \
	boost::weak_ptr<TYPE> TYPE::Get(const std::string& name) { return Engine::Managers::ResourceManager::GetInstance().GetResource<TYPE>(name); }										\
	boost::weak_ptr<TYPE> TYPE::GetByMetadataPath(const std::filesystem::path& path) { return Engine::Managers::ResourceManager::GetInstance().GetResourceByMetadataPath<TYPE>(path); } \
	boost::weak_ptr<TYPE> TYPE::GetByRawPath(const std::filesystem::path& path) { return Engine::Managers::ResourceManager::GetInstance().GetResourceByRawPath<TYPE>(path); }


// Creatable resource creator which infers self as type
#define RESOURCE_SELF_INFER_CREATE_DECL(TYPE)                                     \
    static boost::shared_ptr<TYPE> Create(const std::string& name, const std::filesystem::path& path);

#define RESOURCE_SELF_INFER_CREATE_IMPL(TYPE)									  \
    boost::shared_ptr<TYPE> TYPE::Create(                                         \
    const std::string& name, const std::filesystem::path& path)                   \
    {                                                                             \
        if (const auto pcheck = Engine::Managers::ResourceManager::GetInstance(). \
                               GetResourceByRawPath<TYPE>(path).lock();           \
            const auto ncheck = Engine::Managers::ResourceManager::GetInstance(). \
                               GetResource<TYPE>(name).lock())					  \
        {																		  \
            return ncheck;														  \
        }																		  \
        const auto obj = boost::make_shared<TYPE>(path);						  \
        Engine::Managers::ResourceManager::GetInstance().AddResource(name, obj);  \
        return obj;																  \
    }

namespace Engine 
{
	enum eResourceType : uint8_t
	{
		RES_T_UNK = 0,
		RES_T_SHADER,
		RES_T_TEX, // Broad definition of texture
		RES_T_FONT,
		RES_T_SOUND,
		RES_T_BONE_ANIM,
		RES_T_BONE,
		RES_T_BASE_ANIM,
		RES_T_MTR,
		RES_T_MESH,
		RES_T_SHAPE,
		RES_T_ANIMS_TEX,
		RES_T_COMPUTE_SHADER,
		RES_T_SHADOW_TEX,
		RES_T_PREFAB,
		RES_T_ATLAS_TEX,
		RES_T_ATLAS_ANIM,
		RES_T_MAX,
	};

	template <typename T>
	struct which_resource
	{
		static constexpr eResourceType value = T::rtype;
	};
}

namespace Engine::Abstracts
{
	class Resource : public Entity
	{
	public:
		using type = Resource;
		~Resource() override;

		virtual void Load() final;
		void         Unload();

		void OnDeserialized() override;

		[[nodiscard]] bool IsLoaded() const;
		[[nodiscard]] const std::filesystem::path&        GetPath() const;
		[[nodiscard]] virtual eResourceType GetResourceType() const;

		void SetPath(const std::filesystem::path& path);
		
	protected:
		Resource(std::filesystem::path path, eResourceType type);

		virtual void Load_INTERNAL() = 0;
		virtual void Unload_INTERNAL() = 0;

	private:
		Resource();
		SERIALIZE_DECL
		friend class Managers::ResourceManager;

		bool          m_bLoaded_;
		eResourceType m_type_;
		std::filesystem::path m_path_;
	};
} // namespace Engine::Abstract

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Engine::Abstract::Resource)
