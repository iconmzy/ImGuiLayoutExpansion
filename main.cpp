#include "FrameGUILayout.h"
#include "windows.h"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>


static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;


extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static bool CreateDeviceD3D(HWND hWnd);
static void CleanupDeviceD3D();
static void CreateRenderTarget();
static void CleanupRenderTarget();


static void WinLat() { ImGui::Begin("Latitude");  ImGui::Text("Lat content");  ImGui::End(); }
static void WinLon() { ImGui::Begin("Longitude"); ImGui::Text("Lon content");  ImGui::End(); }
static void WinAlt() { ImGui::Begin("Altitude");  ImGui::Text("Alt content");  ImGui::End(); }
static void WinYaw() { ImGui::Begin("Yaw");       ImGui::Text("Yaw content");  ImGui::End(); }
static void WinPitch() { ImGui::Begin("Pitch");     ImGui::Text("Pitch content"); ImGui::End(); }
static void WinRoll() { ImGui::Begin("Roll");      ImGui::Text("Roll content"); ImGui::End(); }


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, hInstance, nullptr, nullptr, nullptr, nullptr, _T("ImGui Layout Demo"), nullptr };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("Draggable Split Layout Demo"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, hInstance, nullptr);
    if (!CreateDeviceD3D(hwnd)) { CleanupDeviceD3D(); ::UnregisterClass(wc.lpszClassName, hInstance); return 1; }
    ::ShowWindow(hwnd, nCmdShow); ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION(); ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io; io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd); ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    
    
    auto* root = new FrameGUILayout::CustomLayoutNode(false, "RootRows");


    auto* row0 = new FrameGUILayout::CustomLayoutNode(true, "Geodetic");
    row0->SetVerticalChildren(
        new FrameGUILayout::CustomLayoutNode(&WinLat, "Latitude"),
        new FrameGUILayout::CustomLayoutNode(&WinLon, "Longitude"),
        new FrameGUILayout::CustomLayoutNode(&WinAlt, "Altitude")
    );

   
    auto* row1 = new FrameGUILayout::CustomLayoutNode(true, "Attitude");
    row1->SetVerticalChildren(
        new FrameGUILayout::CustomLayoutNode(&WinYaw, "Yaw"),
        new FrameGUILayout::CustomLayoutNode(&WinPitch, "Pitch"),
        new FrameGUILayout::CustomLayoutNode(&WinRoll, "Roll")
    );


    static void (*WinConsole)() = +[]() { ImGui::Begin("Console"); ImGui::TextColored(ImVec4(1, 1, 0, 1), "[INFO] Ready"); ImGui::End(); };
    auto* row2 = new FrameGUILayout::CustomLayoutNode(true, "ConsoleRow");
    row2->SetVerticalChildren(new FrameGUILayout::CustomLayoutNode(WinConsole, "Console"));

    root->AddHorizontalChild(row0);
    root->AddHorizontalChild(row1);
    root->AddHorizontalChild(row2);

    FrameGUILayout::CustomLayout layout(root);

    bool done = false;
    while (!done) {
        MSG msg; while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) { ::TranslateMessage(&msg); ::DispatchMessage(&msg); if (msg.message == WM_QUIT) done = true; }
        if (done) break;
        ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame();

        layout.UpdateAndRender();

        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.1f, 0.1f, 0.1f, 1.00f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext();
    CleanupDeviceD3D(); ::DestroyWindow(hwnd); ::UnregisterClass(wc.lpszClassName, hInstance); return 0;
}

static bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; sd.BufferDesc.RefreshRate.Numerator = 60; sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1; sd.SampleDesc.Quality = 0; sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    UINT flags = 0; D3D_FEATURE_LEVEL fl; const D3D_FEATURE_LEVEL flv[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, flv, 2, D3D11_SDK_VERSION,
        &sd, &g_pSwapChain, &g_pd3dDevice, &fl, &g_pd3dDeviceContext) != S_OK) return false;
    CreateRenderTarget(); return true;
}

static void CleanupDeviceD3D() {
    CleanupRenderTarget(); if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

static void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr; g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView); pBackBuffer->Release();
}

static void CleanupRenderTarget() { if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; } }

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget(); g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0); CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0; break;
    case WM_DESTROY:
        ::PostQuitMessage(0); return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
