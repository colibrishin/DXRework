#pragma once
#include "egResource.h"
#include "egToolkitAPI.h"

namespace Engine
{
  inline constexpr FMOD_VECTOR g_fmod_forward = {0, 0, -1.f};
  inline constexpr FMOD_VECTOR g_fmod_up      = {0, 1, 0.f};
} // namespace Engine

namespace Engine::Resources
{
  class Sound : public Abstract::Resource
  {
  public:
    RESOURCE_T(RES_T_SOUND)
    explicit Sound(const std::filesystem::path& path);

    ~Sound() override;

    void Initialize() override;
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PostUpdate(const float& dt) override;

    void Play(const WeakObjectBase& origin);
    void PlayLoop(const WeakObjectBase& origin);
    bool IsPlaying(const WeakObjectBase& origin);
    void Stop(const WeakObjectBase& origin);
    void StopLoop(const WeakObjectBase& origin);

    void UpdatePosition(const WeakObjectBase& origin);
    void SetRollOff(const FMOD_MODE& roll_off) const;
    void SetMinDistance(const float& min_distance);
    void SetMaxDistance(const float& max_distance);

    void OnSerialized() override;
    void OnDeserialized() override;

    RESOURCE_SELF_INFER_GETTER(Sound)
    RESOURCE_SELF_INFER_CREATE(Sound)

  protected:
    Sound();

    void Load_INTERNAL() override;
    void Unload_INTERNAL() override;
    
  private:
    SERIALIZE_DECL

    void CommitDistance() const;

    void Play_INTERNAL(const WeakObjectBase& origin);

    FMOD::Sound* m_sound_ = nullptr;
    std::map<WeakObjectBase, FMOD::Channel*, WeakComparer<Abstract::ObjectBase>>
    m_channel_map_;
    FMOD_MODE m_mode_;

    float m_min_distance_;
    float m_max_distance_;
  };
} // namespace Engine::Resources

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Sound)
