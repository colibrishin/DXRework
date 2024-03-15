#pragma once
#include "egCommon.hpp"
#include "egRenderable.h"

namespace Engine::Abstract
{
  class Resource : public Renderable
  {
  public:
    using type = Resource;
    ~Resource() override;

    virtual void Load() final;
    void         Unload();

    void OnDeserialized() override;

    bool IsLoaded() const;

    const std::filesystem::path& GetPath() const;
    virtual eResourceType        GetResourceType() const;

    void SetPath(const std::filesystem::path& path);

    void OnImGui() override;

  protected:
    Resource(std::filesystem::path path, eResourceType type);

    virtual void Load_INTERNAL() = 0;
    virtual void Unload_INTERNAL() = 0;

  private:
    Resource();
    SERIALIZE_DECL
    friend class Engine::Manager::ResourceManager;

    bool                  m_bLoaded_;
    std::string           m_path_str_; // for serialization
    eResourceType         m_type_;

    // non-serialized
    std::filesystem::path m_path_;
  };
} // namespace Engine::Abstract

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Engine::Abstract::Resource)
BOOST_CLASS_EXPORT_KEY(Engine::Abstract::Resource)
