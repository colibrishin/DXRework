#include "pch.h"
#include "egMesh.h"
#include <execution>
#include "egManagerHelper.hpp"

SERIALIZE_IMPL
(
 Engine::Resources::Mesh,
 _ARTAG(_BSTSUPER(Engine::Abstract::Resource))
 _ARTAG(m_vertices_) _ARTAG(m_indices_)
 _ARTAG(m_bounding_box_)
)

namespace Engine::Resources
{
  UINT Mesh::GetIndexCount() const { return static_cast<UINT>(m_indices_.size()); }

  const VertexCollection& Mesh::GetVertexCollection() const { return m_vertices_; }

  Mesh::Mesh(const VertexCollection& shape, const IndexCollection& indices)
    : Resource("", RES_T_MESH),
      m_vertices_(shape),
      m_indices_(indices),
      m_vertex_buffer_view_(),
      m_index_buffer_view_() {}

  void __vectorcall Mesh::GenerateTangentBinormal(
    const Vector3& v0, const Vector3&  v1,
    const Vector3& v2, const Vector2&  uv0,
    const Vector2& uv1, const Vector2& uv2,
    Vector3&       tangent, Vector3&   binormal
  )
  {
    const Vector3 edge1 = v1 - v0;
    const Vector3 edge2 = v2 - v0;

    const Vector2 deltaUV1 = {uv1.x - uv0.x, uv2.x - uv0.x};
    const Vector2 deltaUV2 = {uv1.y - uv0.y, uv2.y - uv0.y};

    const float delta       = (deltaUV1.x * deltaUV2.y) - (deltaUV1.y * deltaUV2.x);
    const float denominator = 1.0f / delta;

    tangent.x = denominator * (deltaUV1.y * edge1.x - deltaUV1.x * edge2.x);
    tangent.y = denominator * (deltaUV1.y * edge1.y - deltaUV1.x * edge2.y);
    tangent.z = denominator * (deltaUV1.y * edge1.z - deltaUV1.x * edge2.z);

    binormal.x = denominator * (deltaUV2.x * edge2.x - deltaUV2.y * edge1.x);
    binormal.y = denominator * (deltaUV2.x * edge2.y - deltaUV2.y * edge1.y);
    binormal.z = denominator * (deltaUV2.x * edge2.z - deltaUV2.y * edge1.z);

    tangent.Normalize();
    binormal.Normalize();
  }

  void Mesh::UpdateTangentBinormal()
  {
    struct FacePair
    {
      VertexElement* o[3];
    };

    const auto&           indices = m_indices_;
    if (indices.size() % 3 != 0)
    {
      return;
    }

    std::vector<FacePair> faces;

    for (size_t i = 0; i < indices.size(); i += 3)
    {
      const auto& i0 = indices[i];
      const auto& i1 = indices[i + 1];
      const auto& i2 = indices[i + 2];

      FacePair face = {{&m_vertices_[i0], &m_vertices_[i1], &m_vertices_[i2]}};

      faces.push_back(face);
    }

    std::mutex commit_lock;

    std::for_each
      (
       std::execution::par, faces.begin(), faces.end(),
       [&](const FacePair& face)
       {
         Vector3 tangent;
         Vector3 binormal;

         GenerateTangentBinormal
           (
            face.o[0]->position, face.o[1]->position,
            face.o[2]->position, face.o[0]->texCoord,
            face.o[1]->texCoord, face.o[2]->texCoord, tangent,
            binormal
           );

         {
           std::lock_guard cl(commit_lock);
           face.o[0]->tangent = tangent;
           face.o[1]->tangent = tangent;
           face.o[2]->tangent = tangent;

           face.o[0]->binormal = binormal;
           face.o[1]->binormal = binormal;
           face.o[2]->binormal = binormal;
         }
       }
      );
  }

  void Mesh::PreUpdate(const float& dt) {}

  void Mesh::Update(const float& dt) {}

  void Mesh::FixedUpdate(const float& dt) {}

  void Mesh::OnDeserialized() { Resource::OnDeserialized(); }

  void Mesh::OnSerialized() {}

  D3D12_VERTEX_BUFFER_VIEW Mesh::GetVertexView() const
  {
    return m_vertex_buffer_view_;
  }

  D3D12_INDEX_BUFFER_VIEW Mesh::GetIndexView() const
  {
    return m_index_buffer_view_;
  }

  const AccelStructBuffer& Mesh::GetBLAS() const
  {
    return m_blas_;
  }

  const StructuredBuffer<VertexElement>& Mesh::GetVertexStructuredBuffer() const
  {
    return m_vertex_buffer_structured_;
  }

  ID3D12Resource* Mesh::GetIndexBuffer() const
  {
    return m_index_buffer_.Get();
  }

  void Mesh::Initialize() {}

  void Mesh::PostUpdate(const float& dt) {}

  BoundingBox Mesh::GetBoundingBox() const { return m_bounding_box_; }

  Mesh::Mesh()
    : Resource("", RES_T_MESH),
      m_vertex_buffer_view_(),
      m_index_buffer_view_() {}

  void Mesh::Load_INTERNAL()
  {
    Load_CUSTOM();

    UpdateTangentBinormal();

    std::vector<Vector3> pure_vertices;
    for (const auto& vertex : m_vertices_) { pure_vertices.push_back(vertex.position); }

    BoundingBox::CreateFromPoints
      (
       m_bounding_box_, m_vertices_.size(), pure_vertices.data(),
       sizeof(Vector3)
      );

    std::string generic_name = GetName();

    const std::wstring vertex_name = std::wstring(generic_name.begin(), generic_name.end()) + L"VertexBuffer";

    const auto& cmd = GetD3Device().AcquireCommandPair(L"Mesh Load Command Pair").lock();

    cmd->SoftReset();

    // -- Structured Buffer -- //
    // structured buffer for the raytracing pipeline.
    m_vertex_buffer_structured_.SetData(
        cmd->GetList(), 
        static_cast<UINT>(m_vertices_.size()), 
        m_vertices_.data());

    // Since vertices are not going to be modified, we can transition to SRV and keep it.
    m_vertex_buffer_structured_.TransitionToSRV(cmd->GetList());

    // -- Vertex Buffer -- //
    // Initialize vertex buffer.
    const auto& default_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    const auto& vtx_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(VertexElement) * m_vertices_.size());

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateCommittedResource
       (
        &default_heap,
        D3D12_HEAP_FLAG_NONE,
        &vtx_buffer_desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(m_vertex_buffer_.GetAddressOf())
       )
      );

    DX::ThrowIfFailed(m_vertex_buffer_->SetName(vertex_name.c_str()));

    // -- Upload Buffer -- //
    // Create the upload heap.
    DX::ThrowIfFailed
    (
        DirectX::CreateUploadBuffer
        (
            GetD3Device().GetDevice(),
            m_vertices_.data(),
            m_vertices_.size(),
            m_vertex_buffer_upload_.GetAddressOf()
        )
    );

    // -- Upload Data -- //
    // Copy data to the intermediate upload heap and then schedule a copy from the upload heap to the vertex buffer.
    {
      char* data = nullptr;
      DX::ThrowIfFailed(m_vertex_buffer_upload_->Map(0, nullptr, reinterpret_cast<void**>(&data)));
      _mm256_memcpy(data, m_vertices_.data(), sizeof(VertexElement) * m_vertices_.size());
      m_vertex_buffer_upload_->Unmap(0, nullptr);
    }

    cmd->GetList()->CopyResource(m_vertex_buffer_.Get(), m_vertex_buffer_upload_.Get());

    // -- Vertex Buffer View -- //
    // Initialize vertex buffer view.
    m_vertex_buffer_view_.BufferLocation = m_vertex_buffer_->GetGPUVirtualAddress();
    m_vertex_buffer_view_.SizeInBytes = sizeof(VertexElement) * static_cast<UINT>(m_vertices_.size());
    m_vertex_buffer_view_.StrideInBytes = sizeof(VertexElement);

    const std::wstring index_name = std::wstring(generic_name.begin(), generic_name.end()) + L"IndexBuffer";

    const auto& idx_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT) * m_indices_.size());

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateCommittedResource
       (
        &default_heap,
        D3D12_HEAP_FLAG_NONE,
        &idx_buffer_desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(m_index_buffer_.GetAddressOf())
       )
      );

    DX::ThrowIfFailed(m_index_buffer_->SetName(index_name.c_str()));

    // Create the upload heap.
    DX::ThrowIfFailed
      (
       DirectX::CreateUploadBuffer
       (
        GetD3Device().GetDevice(),
        m_indices_.data(),
        m_indices_.size(),
        m_index_buffer_upload_.GetAddressOf()
       )
      );

    {
      char* data = nullptr;
      DX::ThrowIfFailed(m_index_buffer_upload_->Map(0, nullptr, reinterpret_cast<void**>(&data)));
      _mm256_memcpy(data, m_indices_.data(), sizeof(UINT) * m_indices_.size());
      m_index_buffer_upload_->Unmap(0, nullptr);
    }
    
    cmd->GetList()->CopyResource(m_index_buffer_.Get(), m_index_buffer_upload_.Get());

    const auto& idx_trans = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_index_buffer_.Get(),
       D3D12_RESOURCE_STATE_COPY_DEST, 
          D3D12_RESOURCE_STATE_INDEX_BUFFER
      );

    m_index_buffer_view_.BufferLocation = m_index_buffer_->GetGPUVirtualAddress();
    m_index_buffer_view_.SizeInBytes = sizeof(UINT) * static_cast<UINT>(m_indices_.size());
    m_index_buffer_view_.Format = DXGI_FORMAT_R32_UINT;

    // -- Resource Barrier -- //
    // Transition from copy dest buffer to vertex buffer.
    const auto& vtx_trans = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_vertex_buffer_.Get(),
       D3D12_RESOURCE_STATE_COPY_DEST,
       D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
      );

    cmd->GetList()->ResourceBarrier(1, &vtx_trans);
    cmd->GetList()->ResourceBarrier(1, &idx_trans);

    if (pure_vertices.size() % 3 == 0)
    {
      const auto& vtx_pure_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Vector3) * pure_vertices.size());
      const auto& idx_pure_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT) * m_indices_.size());
      
      DX::ThrowIfFailed
        (
         GetD3Device().GetDevice()->CreateCommittedResource
         (
          &default_heap,
          D3D12_HEAP_FLAG_NONE,
          &vtx_pure_buffer_desc,
          D3D12_RESOURCE_STATE_COPY_DEST,
          nullptr,
          IID_PPV_ARGS(m_raytracing_vertex_buffer_.GetAddressOf())
         )
        );

      DX::ThrowIfFailed
        (
         GetD3Device().GetDevice()->CreateCommittedResource
         (
          &default_heap,
          D3D12_HEAP_FLAG_NONE,
          &idx_pure_buffer_desc,
          D3D12_RESOURCE_STATE_COPY_DEST,
          nullptr,
          IID_PPV_ARGS(m_raytracing_index_buffer_.GetAddressOf())
         )
        );

      DX::ThrowIfFailed
        (
         DirectX::CreateUploadBuffer
         (
          GetD3Device().GetDevice(),
          pure_vertices.data(),
          pure_vertices.size(),
          m_raytracing_vertex_buffer_upload_.GetAddressOf()
         )
        );

      DX::ThrowIfFailed
        (
         DirectX::CreateUploadBuffer
         (
          GetD3Device().GetDevice(),
          m_indices_.data(),
          m_indices_.size(),
          m_raytracing_index_buffer_upload_.GetAddressOf()
         )
        );

      {
        char* data = nullptr;
        DX::ThrowIfFailed(m_raytracing_vertex_buffer_upload_->Map(0, nullptr, reinterpret_cast<void**>(&data)));
        _mm256_memcpy(data, pure_vertices.data(), sizeof(Vector3) * pure_vertices.size());
        m_raytracing_vertex_buffer_upload_->Unmap(0, nullptr);
      }

      {
        char* data = nullptr;
        DX::ThrowIfFailed(m_raytracing_index_buffer_upload_->Map(0, nullptr, reinterpret_cast<void**>(&data)));
        _mm256_memcpy(data, m_indices_.data(), sizeof(UINT) * m_indices_.size());
        m_raytracing_index_buffer_upload_->Unmap(0, nullptr);
      }

      cmd->GetList()->CopyResource(m_raytracing_vertex_buffer_.Get(), m_raytracing_vertex_buffer_upload_.Get());
      cmd->GetList()->CopyResource(m_raytracing_index_buffer_.Get(), m_raytracing_index_buffer_upload_.Get());

      const auto& vtx_pure_trans = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_raytracing_vertex_buffer_.Get(),
       D3D12_RESOURCE_STATE_COPY_DEST,
       D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
      );

      const auto& idx_pure_trans = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_raytracing_index_buffer_.Get(),
       D3D12_RESOURCE_STATE_COPY_DEST,
       D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
      );

      cmd->GetList()->ResourceBarrier(1, &idx_pure_trans);
      cmd->GetList()->ResourceBarrier(1, &vtx_pure_trans);

      // todo: animation deformation
      D3D12_RAYTRACING_GEOMETRY_DESC geo_desc{};
      geo_desc.Type      = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
      geo_desc.Triangles =
      {
        .Transform3x4 = 0,
        .IndexFormat = DXGI_FORMAT_R32_UINT,
        .VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT,
        .IndexCount = GetIndexCount(),
        .VertexCount = static_cast<UINT>(m_vertices_.size()),
        .IndexBuffer = m_raytracing_index_buffer_->GetGPUVirtualAddress(),
        .VertexBuffer = {m_raytracing_vertex_buffer_->GetGPUVirtualAddress(), sizeof(Vector3)}
      };
      geo_desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

      D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS blas_inputs{};
      blas_inputs.Flags          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
      blas_inputs.Type           = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
      blas_inputs.DescsLayout    = D3D12_ELEMENTS_LAYOUT_ARRAY;
      blas_inputs.NumDescs       = 1;
      blas_inputs.pGeometryDescs = &geo_desc;

      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO blas_prebuild_info{};
      GetRaytracingPipeline().GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo
        (&blas_inputs, &blas_prebuild_info);

      if (blas_prebuild_info.ResultDataMaxSizeInBytes == 0)
      {
        throw std::runtime_error("Bottom level acceleration structure prebuild info returned a size of 0.");
      }

      const auto& result_size = Align
        (blas_prebuild_info.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);

      const auto& scratch_size = Align
        (blas_prebuild_info.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);

      const auto& bl_desc = CD3DX12_RESOURCE_DESC::Buffer
        (
         result_size,
         D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
        );

      DX::ThrowIfFailed
        (
         GetD3Device().GetDevice()->CreateCommittedResource
         (
          &default_heap,
          D3D12_HEAP_FLAG_NONE,
          &bl_desc,
          D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
          nullptr,
          IID_PPV_ARGS(m_blas_.result.GetAddressOf())
         )
        );

      const auto& scratch_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer
        (
         scratch_size,
         D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
        );

      DX::ThrowIfFailed
        (
         GetD3Device().GetDevice()->CreateCommittedResource
         (
          &default_heap,
          D3D12_HEAP_FLAG_NONE,
          &scratch_buffer_desc,
          D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
          nullptr,
          IID_PPV_ARGS(m_blas_.scratch.GetAddressOf())
         )
        );

      D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC blas_desc{};

      blas_desc.DestAccelerationStructureData    = m_blas_.result->GetGPUVirtualAddress();
      blas_desc.Inputs                           = blas_inputs;
      blas_desc.ScratchAccelerationStructureData = m_blas_.scratch->GetGPUVirtualAddress();

      cmd->GetList4()->BuildRaytracingAccelerationStructure
        (
         &blas_desc,
         0,
         nullptr
        );

      const auto& uav_barrier = CD3DX12_RESOURCE_BARRIER::UAV(m_blas_.result.Get());
      cmd->GetList()->ResourceBarrier(1, &uav_barrier);

      m_blas_.empty = false;
    }
    else
    {
      m_blas_.empty = true;
    }

    cmd->FlagReady();
  }

  void Mesh::Load_CUSTOM() {}

  void Mesh::Unload_INTERNAL()
  {
    m_vertex_buffer_->Release();
    m_index_buffer_->Release();
    m_vertex_buffer_upload_->Release();
    m_index_buffer_upload_->Release();

    if (m_blas_.result)
    {
      m_blas_.result->Release();
    }
    if (m_blas_.scratch)
    {
      m_blas_.scratch->Release();
    }
    if (m_raytracing_vertex_buffer_)
    {
      m_raytracing_vertex_buffer_->Release();
    }
    if (m_raytracing_index_buffer_)
    {
      m_raytracing_index_buffer_->Release();
    }
    if (m_raytracing_vertex_buffer_upload_)
    {
      m_raytracing_vertex_buffer_upload_->Release();
    }
    if (m_raytracing_index_buffer_upload_)
    {
      m_raytracing_index_buffer_upload_->Release();
    }
    if (m_blas_.instanceDesc)
    {
      m_blas_.instanceDesc->Release();
    }
    
    m_blas_.scratchSize = 0;
    m_blas_.resultSize = 0;
    m_blas_.instanceDescSize = 0;
  }
} // namespace Engine::Resources
