#pragma once
#include "egCommon.hpp"
#include "egDXCommon.h"
#include "egShader.hpp"

namespace Engine::Resources
{
  class RaytracingShader : public Shader
  {
  public:
	  RESOURCE_T(RES_T_RAYTRACING_SHADER)

	  RaytracingShader(const eRTExports exports, const std::vector<RTShaderDefinition>& shader_definitions, const UINT max_recursion);
	  ~RaytracingShader() override = default;

	  void Initialize() override;
	  void PreUpdate(const float& dt) override;
	  void Update(const float& dt) override;
	  void FixedUpdate(const float& dt) override;
	  void PostUpdate(const float& dt) override;

	  [[nodiscard]] size_t                                                   GetHitGroupCount() const;
	  [[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS                                GetRayGenTablePointer() const;
	  [[nodiscard]] size_t                                                   GetRayGenByteSize() const;
	  [[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS                                GetMissTablePointer() const;
	  [[nodiscard]] size_t                                                   GetMissByteSize() const;
	  [[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS                                GetHitTablePointer() const;
	  [[nodiscard]] size_t                                                   GetHitGroupByteSize() const;
	  [[nodiscard]] const std::vector<RTShaderDefinition>& GetHitGroups() const;

	  void UpdateHitShaderRecords(const std::vector<HitShaderRecord>& shader_records);

  protected:
	  void OnDeserialized() override;
	  void OnSerialized() override;

    void Load_INTERNAL() override;
	  void Unload_INTERNAL() override;

  private:
	  RaytracingShader();
	  SERIALIZE_DECL

	  void InitializeSampler();
    void InitializeLocalSignature();
    void CompileShader(ComPtr<IDxcBlob>& blob) const;
    void InitializePipelineState(const ComPtr<IDxcBlob>& blob);
    void InitializeShaderTable();
	  
    eRTExports m_export_flags_;
	  std::vector<RTShaderDefinition> m_shader_definitions_;
	  UINT m_max_recursion_;

	  // non-serialized
		std::vector<RTShaderDefinition> m_hit_shader_definitions_;

	  ComPtr<ID3D12DescriptorHeap> m_sampler_heap_;
	  ComPtr<ID3D12StateObject> m_pipeline_state_object_;
	  ComPtr<ID3D12StateObjectProperties> m_pso_properties_;
	  ComPtr<ID3D12RootSignature> m_local_signature_;

    ComPtr<ID3D12Resource> m_raygen_shader_table_;
    ComPtr<ID3D12Resource> m_hit_shader_table_;
	  size_t m_hit_shader_table_size_;
	  size_t m_hit_shader_table_used_size_;
    ComPtr<ID3D12Resource> m_miss_shader_table_;
  };
}