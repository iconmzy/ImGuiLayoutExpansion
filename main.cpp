#include "windows.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include "CustomDearImGuiLayout.h" 



#define _CRT_SECURE_NO_WARNINGS


static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;


LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


void MainWindowFunc();
void LeftPanelFunc();
void RightPanelFunc();
void ConsoleFunc();


bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, hInstance, nullptr, nullptr, nullptr, nullptr, _T("ImGui Example"), nullptr };
	::RegisterClassEx(&wc);
	HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("Dear ImGui DirectX11 Example"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, hInstance, nullptr);

	
	if (!CreateDeviceD3D(hwnd))
	{
		CleanupDeviceD3D();
		::UnregisterClass(wc.lpszClassName, hInstance);
		return 1;
	}


	::ShowWindow(hwnd, nCmdShow);
	::UpdateWindow(hwnd);


	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; 


	ImGui::StyleColorsDark();


	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);




	auto* root = new FrameGUILayout::CustomLayoutNode(
		true, 
		ImVec2(0, 0),
		ImVec2(0, 0),
		{ 0.3f, 0.7f }  
	);

	auto* topBar = new FrameGUILayout::CustomLayoutNode(false);
	topBar->SetHorizontalChildren(
		new FrameGUILayout::CustomLayoutNode(LeftPanelFunc),
		new FrameGUILayout::CustomLayoutNode(MainWindowFunc),
		new FrameGUILayout::CustomLayoutNode(RightPanelFunc)
	);



	auto* bottom = new FrameGUILayout::CustomLayoutNode(false);
	bottom->SetHorizontalChildren(
		new FrameGUILayout::CustomLayoutNode(ConsoleFunc)
	);

	root->AddVerticalChild(topBar);
	root->AddVerticalChild(bottom);

	FrameGUILayout::CustomLayout layout(root);

	bool done = false;
	while (!done)
	{

		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}
		if (done)
			break;


		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		layout.UpdateAndRender();


		ImGui::Render();
		const float clear_color_with_alpha[4] = { 0.45f, 0.55f, 0.60f, 1.00f };
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());


		g_pSwapChain->Present(1, 0);
	}


	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClass(wc.lpszClassName, hInstance);

	return 0;
}



void MainWindowFunc()
{
	ImGui::Begin("Main Content");
	ImGui::Text("This is the main content area");
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	static float f = 0.0f;
	ImGui::SliderFloat("Float", &f, 0.0f, 1.0f);
	ImGui::End();
}

void LeftPanelFunc()
{
	ImGui::Begin("Left Panel");
	ImGui::Text("This is the left panel");

	static int counter = 0;
	if (ImGui::Button("Click me!"))
		counter++;
	ImGui::SameLine();
	ImGui::Text("Counter = %d", counter);


	static float color[3] = { 1.0f, 0.0f, 0.0f };
	ImGui::ColorEdit3("Color", color);

	ImGui::End();
}

void RightPanelFunc()
{
	ImGui::Begin("Right Panel");
	ImGui::Text("This is the right panel");


	static int selected = 0;
	ImGui::Text("Items:");
	for (int i = 0; i < 10; i++)
	{
		char label[32];
		sprintf_s(label, sizeof(label), "Item %d", i); 
		if (ImGui::Selectable(label, selected == i))
			selected = i;
	}

	ImGui::End();
}

void ConsoleFunc()
{
	ImGui::Begin("Console");
	ImGui::Text("This is the console area");


	ImGui::TextColored(ImVec4(1, 1, 0, 1), "[INFO] Application started");
	ImGui::TextColored(ImVec4(0, 1, 0, 1), "[SUCCESS] Initialized all systems");
	ImGui::TextColored(ImVec4(1, 0, 0, 1), "[ERROR] Could not connect to server");

	static char input[128] = "";
	ImGui::InputText("Command", input, IM_ARRAYSIZE(input));
	if (ImGui::Button("Send"))
	{
		// 处理命令
		ImGui::Text("> %s", input);
		input[0] = '\0';
	}

	ImGui::End();
}


LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
		{
			CleanupRenderTarget();
			g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
			CreateRenderTarget();
		}
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) 
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

bool CreateDeviceD3D(HWND hWnd)
{
	
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;

	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
		return false;

	CreateRenderTarget();
	return true;
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
	if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
	pBackBuffer->Release();
}

void CleanupRenderTarget()
{
	if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}