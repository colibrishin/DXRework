#pragma once
#include "Source/Runtime/Abstracts/CoreSingleton/Public/Singleton.hpp"

#if defined(USE_DX12)
#include <directxtk12/BufferHelpers.h>
#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include "Source/Runtime/DescriptorHeap/Public/Descriptors.h"
#include "Source/Runtime/ConstantBufferDX12/Public/ConstantBuffer.hpp"
#endif

namespace Engine::Graphics 
{
	struct ParamBase
	{
	public:
		constexpr ParamBase() = default;

		template <typename T>
		void SetParam(const size_t slot, const T& param)
		{
			if constexpr (std::is_same_v<T, int>)
			{
				i_param[slot] = param;
			}
			else if constexpr (std::is_same_v<T, UINT>)
			{
				i_param[slot] = static_cast<int>(param);
			}
			else if constexpr (std::is_same_v<T, float>)
			{
				f_param[slot] = param;
			}
			else if constexpr (std::is_same_v<T, Vector3>)
			{
				std::memcpy(&v_param[slot], &param, sizeof(Vector3));
			}
			else if constexpr (std::is_same_v<T, Vector4>)
			{
				_mm_store_ps(v_param[slot].x.m128_f32, _mm_load_ps(param.x));
			}
			else if constexpr (std::is_same_v<T, Matrix>)
			{
				const auto row0 = const_cast<float*>(&param.m[0][0]);
				const auto row2 = const_cast<float*>(&param.m[2][0]);

				_mm256_store_ps(m_param[slot].m[0], _mm256_load_ps(row0));
				_mm256_store_ps(m_param[slot].m[2], _mm256_load_ps(row2));
			}
			else
			{
				throw std::runtime_error("Invalid type");
			}
		}

		template <typename T>
		T& GetParam(const size_t slot)
		{
			if constexpr (std::is_same_v<T, int>)
			{
				return i_param[slot];
			}
			else if constexpr (std::is_same_v<T, UINT>)
			{
				return reinterpret_cast<UINT&>(i_param[slot]);
			}
			else if constexpr (std::is_same_v<T, bool>)
			{
				return reinterpret_cast<bool&>(i_param[slot]);
			}
			else if constexpr (std::is_same_v<T, float>)
			{
				return f_param[slot];
			}
			else if constexpr (std::is_same_v<T, Vector3>)
			{
				return reinterpret_cast<Vector3&>(v_param[slot]);
			}
			else if constexpr (std::is_same_v<T, Vector4>)
			{
				return reinterpret_cast<Vector4&>(v_param[slot]);
			}
			else if constexpr (std::is_same_v<T, Matrix>)
			{
				return reinterpret_cast<Matrix&>(m_param[slot]);
			}
			else
			{
				throw std::runtime_error("Invalid type");
			}
		}

		template <typename T>
		T GetParam(const size_t slot) const
		{
			if constexpr (std::is_same_v<T, int>)
			{
				return i_param[slot];
			}
			else if constexpr (std::is_same_v<T, bool>)
			{
				return static_cast<bool>(i_param[slot]);
			}
			else if constexpr (std::is_same_v<T, UINT>)
			{
				return static_cast<UINT>(i_param[slot]);
			}
			else if constexpr (std::is_same_v<T, float>)
			{
				return f_param[slot];
			}
			else if constexpr (std::is_same_v<T, Vector3>)
			{
				return v_param[slot];
			}
			else if constexpr (std::is_same_v<T, Vector4>)
			{
				return v_param[slot];
			}
			else if constexpr (std::is_same_v<T, Matrix>)
			{
				Matrix m;
				_mm256_store_ps(m.m[0], m_param[slot].m[0]);
				_mm256_store_ps(m.m[2], m_param[slot].m[1]);
				return m;
			}
			else
			{
				throw std::runtime_error("Invalid type");
			}
		}

	private:
		friend class boost::serialization::access;

		template <class Archive>
		void serialize(Archive& ar, const unsigned int file_version)
		{
			ar& f_param;
			ar& i_param;
			ar& v_param;
			ar& m_param;
		}

		constexpr static size_t max_param = 8;

		float            f_param[max_param * (sizeof(Vector4) / sizeof(float))]{};
		int              i_param[max_param * (sizeof(Vector4) / sizeof(float))]{};
		Vector4 v_param[max_param]{};
		Matrix  m_param[max_param]{};
	};

	static_assert(sizeof(ParamBase) % sizeof(Vector4) == 0);
	static_assert(sizeof(ParamBase) < 2048);

	namespace CBs
	{
		struct ParamCB : public ParamBase
		{
			CB_T(CB_TYPE_PARAM)
		};

		static_assert(sizeof(ParamCB) % sizeof(Vector4) == 0);

		struct ViewportCB
		{
			RT_CB_T(RAYTRACING_CB_VIEWPORT)
				Vector2 resolution;
		};
	}

	namespace SBs
	{
		struct LocalParamSB : public ParamBase
		{
			SB_T(SB_TYPE_LOCAL_PARAM)
		};

		struct InstanceSB : public ParamBase
		{
			SB_T(SB_TYPE_INSTANCE)
				SB_UAV_T(SB_TYPE_UAV_INSTANCE)

		private:
			friend class boost::serialization::access;

			template <class Archive>
			void serialize(Archive& ar, const unsigned int file_version)
			{
				ar& boost::serialization::base_object<ParamBase>(*this);
			}
		};
	}

}

namespace Engine::Managers
{
	using namespace Engine::Graphics;

	class RenderPipeline final : public Abstracts::Singleton<RenderPipeline>
	{
	private:
		struct TempParamTicket
		{
			TempParamTicket(const CBs::ParamCB& previousParam)
				: previousParam(previousParam) {}

			~TempParamTicket()
			{
				Managers::RenderPipeline::GetInstance().m_param_buffer_ = previousParam;
				Managers::RenderPipeline::GetInstance().m_param_buffer_data_.SetData(&previousParam);
			}

		private:
			const CBs::ParamCB previousParam;
		};

	public:
		explicit RenderPipeline(SINGLETON_LOCK_TOKEN) {}

		void Initialize() override;
		void PreRender(const float& dt) override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void Render(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostRender(const float& dt) override;
		void PostUpdate(const float& dt) override;

		void SetPerspectiveMatrix(const CBs::PerspectiveCB& matrix);

		template <typename T>
		void SetParam(const T& v, const size_t slot)
		{
			m_param_buffer_.SetParam(slot, v);
			m_param_buffer_data_.SetData(&m_param_buffer_);
		}

		[[nodiscard]] TempParamTicket SetParam(const ParamBase& param)
		{
			return {m_param_buffer_};
		}

		void DefaultViewport(const Weak<CommandPair>& w_cmd) const;
		void DefaultScissorRect(const Weak<CommandPair>& w_cmd) const;
		void DefaultRootSignature(const Weak<CommandPair>& w_cmd) const;

		ID3D12RootSignature* GetRootSignature() const;

		D3D12_VIEWPORT GetViewport() const;
		D3D12_RECT     GetScissorRect() const;

		static void SetPSO(const Weak<CommandPair>& w_cmd, const Strong<Resources::Shader>& Shader);

		[[nodiscard]] DescriptorPtr AcquireHeapSlot();

		UINT GetBufferDescriptorSize() const;
		UINT GetSamplerDescriptorSize() const;

		void BindConstantBuffers(const Weak<CommandPair>& w_cmd, const DescriptorPtr& heap);
		void BindConstantBuffers(ID3D12GraphicsCommandList1* cmd, const DescriptorPtr& heap);

	private:
		friend class ToolkitAPI;
		friend class D3Device;

		friend struct SingletonDeleter;
		RenderPipeline() = default;
		~RenderPipeline() override;

		void PrecompileShaders();
		void InitializeRootSignature();
		void InitializeNullDescriptors();
		void InitializeHeaps();
		void InitializeStaticBuffers();
		void InitializeViewport();

		ComPtr<ID3D12RootSignature> m_root_signature_ = nullptr;
		ComPtr<ID3D12PipelineState> m_pipeline_state_ = nullptr;

		UINT m_buffer_descriptor_size_  = 0;
		UINT m_sampler_descriptor_size_ = 0;

		std::mutex        m_descriptor_mutex_;
		DescriptorHandler m_descriptor_handler_;

		ComPtr<ID3D12DescriptorHeap> m_null_srv_heap_;
		ComPtr<ID3D12DescriptorHeap> m_null_sampler_heap_;
		ComPtr<ID3D12DescriptorHeap> m_null_cbv_heap_;
		ComPtr<ID3D12DescriptorHeap> m_null_uav_heap_;
		ComPtr<ID3D12DescriptorHeap> m_null_rtv_heap_;
		ComPtr<ID3D12DescriptorHeap> m_null_dsv_heap_;

		D3D12_VIEWPORT m_viewport_{};
		D3D12_RECT     m_scissor_rect_{};

		Graphics::CBs::PerspectiveCB m_wvp_buffer_;
		Graphics::CBs::ParamCB       m_param_buffer_;

		Graphics::ConstantBuffer<CBs::PerspectiveCB> m_wvp_buffer_data_{};
		Graphics::ConstantBuffer<CBs::ParamCB>       m_param_buffer_data_{};

		Strong<Resources::Shader> m_fallback_shader_;
	};
} // namespace Engine::Manager::Graphics
