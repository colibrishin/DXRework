#pragma once
#include "egCommon.hpp"
#include "egRenderable.h"

namespace Engine::Abstract
{
  class Resource : public Renderable
  {
  public:
    ~Resource() override;

    virtual void Load() final;
    void         Unload();

    void OnDeserialized() override;

    bool IsLoaded() const;

    const std::filesystem::path& GetPath() const;
    virtual eResourceType        GetResourceType() const;

    void SetPath(const std::filesystem::path& path);
    LocalResourceID GetLocalID() const;

    void OnImGui() override;

  protected:
    Resource(std::filesystem::path path, eResourceType type);

    virtual void Load_INTERNAL() = 0;
    virtual void Unload_INTERNAL() = 0;

  private:
    SERIALIZER_ACCESS

  private:
    friend class Engine::Manager::ResourceManager;

    bool                  m_bLoaded_;
    std::filesystem::path m_path_;
    std::string           m_path_str_; // for serialization
    eResourceType         m_type_;
    LocalResourceID       m_local_id_;
  };
} // namespace Engine::Abstract

BOOST_CLASS_EXPORT_KEY(Engine::Abstract::Resource)
