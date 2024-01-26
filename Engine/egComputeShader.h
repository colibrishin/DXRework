#pragma once
#include "egMacro.h"
#include "egShader.hpp"
#include "egResourceManager.hpp"

namespace Engine::Resources
{
  class ComputeShader : public Shader
  {
  public:
    virtual ~ComputeShader() override = default;

    void SetGroup(const std::array<UINT, 3>& group);
    std::array<UINT, 3> GetThread() const;
    void Dispatch();

    template <typename T, typename CSLock = std::enable_if_t<std::is_base_of_v<ComputeShader, T>>>
    static boost::weak_ptr<T> Create()
    {
      const auto& v = boost::make_shared<T>();
      v->Initialize();
      v->Load();
      GetResourceManager().AddResource(v);
      return v;
    }

    static inline boost::weak_ptr<ComputeShader> Get(const std::string& name)
    {
      return GetResourceManager().GetResource<ComputeShader>(name).lock();
    }

	protected:
    ComputeShader(const std::string& name, const std::filesystem::path& path, const std::array<UINT, 3>& thread);

    virtual void preDispatch() = 0;
    virtual void postDispatch() = 0;

  private:
    void PostRender(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void PreRender(const float& dt) override;
    void PreUpdate(const float& dt) override;
    void Render(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void Initialize() override;

    void Load_INTERNAL() override;
    void Unload_INTERNAL() override;

  private:
    SERIALIZER_ACCESS
    ComputeShader();

    ComPtr<ID3D11ComputeShader> m_cs_;

    UINT m_thread_[3];
    UINT m_group_[3];

  };
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::ComputeShader)