#include "ImGuiManager.h"
#if WITH_EDITOR
#include "GraphicInterface.h"
#include "imgui.h"
#include "imgui_stdlib.h"



#if USE_DX12
#include "imgui_impl_dx12.h"
#include "Source/Runtime/D3D12GraphicInterface/Public/CommandPair.h"
#endif

#if PLATFORM == Windows
#include "WinAPIWrapper.hpp"
#include "imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND   hWnd,
	UINT   msg,
	WPARAM wParam,
	LPARAM lParam
);
#endif

#include "CoreModule/Public/CoreModule.h"

#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"

namespace Engine::Managers
{
	void ImGuiManager::Initialize()
	{
#if WITH_EDITOR
		UIInterfaceAccessor::SetInterface<ImGuiUIInterface>();
		WinAPI::WinAPIWrapper::RegisterHandler("ImGuiManager", ImGui_ImplWin32_WndProcHandler);
		
		m_imgui_descriptor_ = GraphicInterfaceAccessor::GetInterface().GetHeap();

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

#if PLATFORM == Windows
		ImGui_ImplWin32_Init(WinAPI::WinAPIWrapper::GetHWND());
#endif
		
#if USE_DX12
		const auto& cpu_handle = *static_cast<D3D12_CPU_DESCRIPTOR_HANDLE*>(m_imgui_descriptor_->GetNativeCPUHandle());
		const auto& gpu_handle = *static_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(m_imgui_descriptor_->GetNativeGPUHandle());
		
		ImGui_ImplDX12_Init
				(
				 static_cast<ID3D12Device2*>(GraphicInterfaceAccessor::GetInterface().GetNativeInterface()),
				 CFG_FRAME_BUFFER,
				 DXGI_FORMAT_R8G8B8A8_UNORM,
				 static_cast<ID3D12DescriptorHeap*>(m_imgui_descriptor_->GetNativeHeap()),
				 cpu_handle,
				 gpu_handle
				);
#endif
		
#endif
	}

	void ImGuiManager::OnUIUpdate(UIContext* const parent, const float dt)
	{
		Singleton::OnUIUpdate(parent, dt);
	}

	void ImGuiManager::PreUpdate(const float dt) {}

	void ImGuiManager::PreRender(const float dt) {}

	void ImGuiManager::Render(const float dt)
	{
#if WITH_EDITOR
		ImGui::Render();

		GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();
		const GraphicInterfaceContextReturnType& context = gi.GetNewContext(0, false, L"ImGui Rendering");
		const GraphicInterfaceContextPrimitive& primitive = context.GetPointers();

		primitive.commandList->SoftReset();
		gi.SetDefaultRenderTarget(&primitive);
		gi.SetViewport(&primitive, RenderPipeline::GetInstance().GetViewport());
		m_imgui_descriptor_->BindGraphic(&primitive);

#if USE_DX12
		const auto cmd = static_cast<CommandPair*>(primitive.commandList);
		
		ImGui_ImplDX12_RenderDrawData
				(
				 ImGui::GetDrawData(),
				 cmd->GetList()
				);
#endif
		
		primitive.commandList->FlagReady();
        UIDialogMapper::Clear();
#endif
	}

	void ImGuiManager::PostRender(const float dt) {}

	void ImGuiManager::FixedUpdate(const float dt) {}

	void ImGuiManager::PostUpdate(const float dt) {}
	
	ImGuiManager::~ImGuiManager()
	{
#if WITH_EDITOR
#if USE_DX12
		ImGui_ImplDX12_Shutdown();
#endif

#if PLATFORM == Windows
		ImGui_ImplWin32_Shutdown();
#endif
		
		ImGui::DestroyContext();
#endif
	}

	void ImGuiManager::Update(const float dt) {}
}

void Engine::AlignText(const std::string_view label)
{
	const float width = ImGui::CalcItemWidth();
	const float x     = ImGui::GetCursorPosX();

	ImGui::Text(label.data());
	ImGui::SameLine();
	ImGui::SetCursorPosX(x + width * 0.75f + ImGui::GetStyle().ItemInnerSpacing.x);
	ImGui::SetNextItemWidth(100.f);
}

std::string Engine::LabelSuffix(const std::string_view label)
{
	std::string labelID = "##";
	labelID += label;

	return labelID;
}

void Engine::ImGuiMainMenuBarToken::End()
{
	ImGui::EndMainMenuBar();
}

bool Engine::ImGuiMainMenuBarToken::DoImpl()
{
	return ImGui::BeginMainMenuBar();
}

void Engine::ImGuiMenuToken::End()
{
	ImGui::EndMenu();
}

bool Engine::ImGuiMenuToken::DoImpl(const std::string_view title)
{
	const std::string& temp_label = std::string(title) + LabelSuffix(title);
	return ImGui::BeginMenu(temp_label.c_str());
}

void Engine::ImGuiMenuItemToken::End() {}

bool Engine::ImGuiMenuItemToken::DoImpl(const std::string_view label)
{
	const std::string& temp_label = std::string(label) + LabelSuffix(label);
	return ImGui::MenuItem(temp_label.c_str());
}

void Engine::ImGuiDialogToken::End()
{
	ImGui::End();
}

bool Engine::ImGuiDialogToken::DoImpl( const void * ptr, const std::string_view title, bool &opened )
{
    value = UIDialogMapper::Map( ptr );
    const std::string &value_str  = std::to_string( value );
	const std::string& temp_label = std::string(title) + LabelSuffix(value_str);
	return ImGui::Begin(temp_label.c_str(), &opened, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
}

void Engine::ImGuiButtonToken::End() {}

bool Engine::ImGuiButtonToken::DoImpl(const void* ptr, const std::string_view title)
{
    const uint64_t value = UIDialogMapper::Map( ptr );
    const std::string &value_str  = std::to_string( value );
    const std::string &temp_label = std::string( title ) + LabelSuffix( value_str );
    return ImGui::Button( temp_label.c_str(), { 0, 20 } );
}

void Engine::ImGuiLabelAndTextToken::End() {}

bool Engine::ImGuiLabelAndTextToken::DoImpl(const std::string_view label, std::string& text, const bool editable)
{
	const std::string& temp_label = LabelSuffix(label);
	AlignText(label);
	return ImGui::InputText(temp_label.c_str(), &text, !editable ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None);
}

void Engine::ImGuiLabelAndFloatToken::End()
{
}

bool Engine::ImGuiLabelAndFloatToken::DoImpl(const std::string_view label, float& value, const float speed, const float min, const float max, const bool editable)
{
	const std::string& temp_label = LabelSuffix(label);
	AlignText(label);
	return ImGui::DragFloat(temp_label.c_str(), &value, speed, min, max, "%.3f", !editable ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None);
}

void Engine::ImGuiLabelAndIntToken::End() {}

bool Engine::ImGuiLabelAndIntToken::DoImpl(const std::string_view label, int& value, const float speed, const int min, const int max, bool editable)
{
	const std::string& temp_label = LabelSuffix(label);
	AlignText(label);
	return ImGui::DragScalar(temp_label.c_str(), ImGuiDataType_S32, &value, speed, &min, &max, nullptr, !editable ? ImGuiSliderFlags_NoInput : ImGuiInputTextFlags_None);
}

void Engine::ImGuiLabelAndUIntToken::End() {}

bool Engine::ImGuiLabelAndUIntToken::DoImpl(const std::string_view label, uint32_t& value, const float speed, const uint32_t min, const uint32_t max, bool editable)
{
	const std::string& temp_label = LabelSuffix(label);
	AlignText(label);
	return ImGui::DragScalar(temp_label.c_str(), ImGuiDataType_U32, &value, speed, &min, &max, nullptr, !editable ? ImGuiSliderFlags_NoInput : ImGuiInputTextFlags_None);
}

void Engine::ImGuiLabelAndULLDToken::End() {}

bool Engine::ImGuiLabelAndULLDToken::DoImpl(const std::string_view label, uint64_t& value, const float speed, const uint64_t min, const uint64_t max, bool editable)
{
	const std::string& temp_label = LabelSuffix(label);
	AlignText(label);
	return ImGui::DragScalar(temp_label.c_str(), ImGuiDataType_U64, &value, speed, &min, &max, nullptr, !editable ? ImGuiSliderFlags_NoInput : ImGuiInputTextFlags_None);
}

void Engine::ImGuiLabelAndPathToken::End() {}

bool Engine::ImGuiLabelAndPathToken::DoImpl(const std::string_view label, const std::filesystem::path& path)
{
	const std::string& temp_label = LabelSuffix(label);
	AlignText(label);
	return ImGui::InputText(temp_label.c_str(), const_cast<char*>(path.generic_string().c_str()), ImGuiInputTextFlags_ReadOnly);
}

void Engine::ImGuiListBoxToken::End()
{
	ImGui::EndListBox();
}

bool Engine::ImGuiListBoxToken::DoImpl(const std::string_view label, float x, float y)
{
	return ImGui::BeginListBox(label.data(), {x, y});
}

void Engine::ImGuiTreeNodeToken::End()
{
	ImGui::TreePop();
}

bool Engine::ImGuiTreeNodeToken::DoImpl(const std::string_view label)
{
	const std::string& temp_label = LabelSuffix(label);
	return ImGui::TreeNode(temp_label.c_str(), label.data());
}

void Engine::ImGuiSelectableToken::End()
{
}

bool Engine::ImGuiSelectableToken::DoImpl(const std::string_view label, bool& opened)
{
	return ImGui::Selectable(label.data(), &opened);
}

void Engine::ImGuiLabelAndVec3Token::End()
{
}

bool Engine::ImGuiLabelAndVec3Token::DoImpl(const std::string_view label, float* vec, float step, float min, float max, bool editable)
{
	return ImGui::DragFloat3(label.data(), vec, step, min, max, "%.3f", !editable ? ImGuiSliderFlags_NoInput : ImGuiSliderFlags_None);
}

void Engine::ImGuiCheckboxToken::End()
{
}
bool Engine::ImGuiCheckboxToken::DoImpl(const std::string_view label, bool& flag)
{
	AlignText(label);
	const std::string& temp_label = LabelSuffix(label);
	return ImGui::Checkbox(temp_label.data(), &flag);
}

void Engine::ImGuiComboboxToken::End()
{
}
bool Engine::ImGuiComboboxToken::DoImpl(const std::string_view label, int* value, const char* const* label_arr, const size_t arr_size, const bool editable)
{
	if (!editable)
	{
		ImGui::BeginDisabled();
	}
	bool retval = ImGui::Combo(label.data(), value, label_arr, arr_size);
	if (!editable)
	{
		ImGui::EndDisabled();
	}
	return retval;
}

void Engine::ImGuiLabelAndVec4Token::End() {}

bool Engine::ImGuiLabelAndVec4Token::DoImpl(const std::string_view label, float* vec, float step, float min, float max, bool editable)
{
	return ImGui::DragFloat4(label.data(), vec, step, min, max, "%.3f", !editable ? ImGuiSliderFlags_NoInput : ImGuiSliderFlags_None);
}

void Engine::ImGuiLabelAndVec2Token::End() {}

bool Engine::ImGuiLabelAndVec2Token::DoImpl(const std::string_view label, float* vec, float step, float min, float max, bool editable)
{
	return ImGui::DragFloat4(label.data(), vec, step, min, max, "%.3f", !editable ? ImGuiSliderFlags_NoInput : ImGuiSliderFlags_None);
}

void Engine::ImGuiDragAndDropTargetToken::End()
{
	return ImGui::EndDragDropTarget();
}

bool Engine::ImGuiDragAndDropTargetToken::DoImpl(const std::string_view tag, const std::function<void(void*)> functor)
{
	const bool ret = ImGui::BeginDragDropTarget();

	if (ret)
	{
		if (const auto payload = ImGui::AcceptDragDropPayload(tag.data()))
		{
			if (functor)
			{
				functor(payload->Data);	
			}
		}
	}

	return ret;
}

void Engine::ImGuiDragAndDropSourceToken::End()
{
	ImGui::EndDragDropSource();
}

bool Engine::ImGuiDragAndDropSourceToken::DoImpl(
	const std::string_view tag, const std::string_view label, const void* ptr, unsigned long long size
)
{
	const bool ret = ImGui::BeginDragDropSource();

	if (ret)
	{
		ImGui::SetDragDropPayload(tag.data(), ptr, size);
		ImGui::Text(label.data());
	}

	return ret;
}

void Engine::ImGuiLabelAndUInt16Token::End()
{
}

bool Engine::ImGuiLabelAndUInt16Token::DoImpl(
	const std::string_view label, unsigned short& value, float speed, unsigned short min , unsigned short max, bool editable
)
{
	const std::string& temp_label = LabelSuffix(label);
	AlignText(label);
	return ImGui::DragScalar(temp_label.c_str(), ImGuiDataType_U16, &value, speed, &min, &max, nullptr, !editable ? ImGuiSliderFlags_NoInput : ImGuiInputTextFlags_None);
}

void Engine::ImGuiComboboxUInt8Token::End() {}

bool Engine::ImGuiComboboxUInt8Token::DoImpl(
	const std::string_view label, unsigned char* value, const char* const* label_arr, const unsigned long long arr_size, const bool editable
)
{
	if (!editable)
	{
		ImGui::BeginDisabled();
	}

	int intermediate = *value;
	const bool ret = ImGui::Combo(label.data(), &intermediate, label_arr, arr_size);
	*value = intermediate;

	if (!editable)
	{
		ImGui::EndDisabled();
	}
	return ret;
}

void Engine::ImGuiTextToken::End()
{
}
bool Engine::ImGuiTextToken::DoImpl(const std::string_view text)
{
	ImGui::Text(text.data());
	return true;
}

void Engine::ImGuiSeparatorToken::End()
{
}
bool Engine::ImGuiSeparatorToken::DoImpl()
{
	ImGui::Separator();
	return true;
}

void Engine::ImGuiSameLineToken::End()
{
}

bool Engine::ImGuiSameLineToken::DoImpl()
{
    ImGui::SameLine();
    return true;
}

void Engine::ImGuiTableToken::End()
{
	ImGui::EndTable();
}

bool Engine::ImGuiTableToken::DoImpl(std::string_view label, unsigned long long column_count)
{
	return ImGui::BeginTable(label.data(), column_count);
}
void Engine::ImGuiTableRowToken::End() {}

bool Engine::ImGuiTableRowToken::DoImpl()
{
	ImGui::TableNextRow();
	return true;
}

void Engine::ImGuiTableColumnToken::End() {}

bool Engine::ImGuiTableColumnToken::DoImpl(unsigned long long column)
{
	ImGui::TableNextColumn();
	return true;
}

void Engine::ImGuiUIInterface::NewFrame()
{
#if WITH_EDITOR
		
#if USE_DX12
	ImGui_ImplDX12_NewFrame();
#endif
		
#if PLATFORM == Windows
	ImGui_ImplWin32_NewFrame();
#endif
		
	ImGui::NewFrame();
#endif
}
#endif