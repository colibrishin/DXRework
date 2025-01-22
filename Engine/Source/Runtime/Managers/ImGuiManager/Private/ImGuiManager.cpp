#include "ImGuiManager.h"

#include "GraphicInterface.h"
#include "imgui.h"
#include "imgui_stdlib.h"

#include "ModuleManager/Public/ModuleManager.h"

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

#include "CoreModuel/Public/CoreModule.h"

#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"

MODULE_IMPL(Engine::ImGuiManagerModule, ImGuiManager)

namespace Engine::Managers
{
	void ImGuiManager::Initialize()
	{
#if WITH_EDITOR
		UIInterfaceAccessor::SetInterface<ImGuiUIInterface>();
		WinAPI::WinAPIWrapper::RegisterHandler(ImGui_ImplWin32_WndProcHandler);
		
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

	void ImGuiManager::OnUIUpdate(const float dt) {}

	void ImGuiManager::PreUpdate(const float dt)
	{
#if WITH_EDITOR
		// Start ImGui Routine
#endif
	}

	void ImGuiManager::PreRender(const float dt)
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
#endif
	}

	void ImGuiManager::Render(const float dt) {}

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
	ImGui::SetNextItemWidth(-1);
}

std::string Engine::LabelPrefix(const std::string_view label)
{
	std::string labelID = "##";
	labelID += label;

	return labelID;
}

void Engine::ImGuiMainMenuBarToken::End() const
{
	ImGui::EndMainMenuBar();
}

bool Engine::ImGuiMainMenuBarToken::DoImpl() const
{
	return ImGui::BeginMainMenuBar();
}

void Engine::ImGuiMenuToken::End() const
{
	ImGui::EndMenu();
}

bool Engine::ImGuiMenuToken::DoImpl(const std::string_view title) const
{
	return ImGui::BeginMenu(title.data());
}

void Engine::ImGuiMenuItemToken::End() const {}

bool Engine::ImGuiMenuItemToken::DoImpl(const std::string_view label) const
{
	return ImGui::MenuItem(label.data());
}

void Engine::ImGuiDialogToken::End() const
{
	ImGui::End();
}

bool Engine::ImGuiDialogToken::DoImpl(const std::string_view title, bool& opened) const
{
	return ImGui::Begin(title.data(), &opened);
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

void Engine::ImGuiManagerModule::Initialize()
{
	CoreModule::GetContext().AddManager(
		CoreLoop::LOOP_TYPE_RENDER,
		Managers::ImGuiManager::GetInstance);
}

void Engine::ImGuiManagerModule::Shutdown()
{
	CoreModule::GetContext().RemoveManager(
		CoreLoop::LOOP_TYPE_RENDER,
		Managers::ImGuiManager::GetInstance);
}

bool Engine::ImGuiManagerModule::DynamicLoadable()
{
	return true;	
}
