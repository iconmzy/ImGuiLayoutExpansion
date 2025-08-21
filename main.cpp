#pragma once
#include "FrameGUILayout.h"
#include "windows.h"
#include "imgui.h"
#include "implot.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <cmath> 
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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


static void WinLat() {
    ImGui::Begin("Latitude");
    ImGui::Text("Lat content"); 
    ImGui::End();
}
static void WinLon() { ImGui::Begin("Longitude"); ImGui::Text("Lon content");  ImGui::End(); }
static void WinAlt() { ImGui::Begin("Altitude");  ImGui::Text("Alt content");  ImGui::End(); }
static void WinYaw() { ImGui::Begin("Yaw");       ImGui::Text("Yaw content");  ImGui::End(); }
static void WinPitch() { ImGui::Begin("Pitch");     ImGui::Text("Pitch content"); ImGui::End(); }
static void WinRoll() { ImGui::Begin("Roll");      ImGui::Text("Roll content"); ImGui::End(); }
static void RealtimePlots() {
    ImGui::Begin("realtime Plot");
    ImVec2 avail_size = ImGui::GetContentRegionAvail();
    float plot_height = avail_size.y * 0.5f;
    ImGui::BulletText("Move your mouse to change the data!");
    //ImGui::BulletText("This example assumes 60 FPS. Higher FPS requires larger buffer size.");
    static FrameGUILayout::ScrollingBuffer sdata1, sdata2;
    static FrameGUILayout::RollingBuffer   rdata1, rdata2;
    ImVec2 mouse = ImGui::GetMousePos();
    static float t = 0;
    t += ImGui::GetIO().DeltaTime;
    sdata1.AddPoint(t, mouse.x * 0.0005f);
    rdata1.AddPoint(t, mouse.x * 0.0005f);
    sdata2.AddPoint(t, mouse.y * 0.0005f);
    rdata2.AddPoint(t, mouse.y * 0.0005f);

    static float history = 10.0f;
    ImGui::SliderFloat("History", &history, 1, 30, "%.1f s");
    rdata1.Span = history;
    rdata2.Span = history;

    static ImPlotAxisFlags flags = ImPlotAxisFlags_NoTickLabels;

    if (ImPlot::BeginPlot("##Scrolling", ImVec2(-1, plot_height))) {
        ImPlot::SetupAxes(nullptr, nullptr, flags, flags);
        ImPlot::SetupAxisLimits(ImAxis_X1, t - history, t, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1);
        ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
        
        ImPlot::PlotShaded("Mouse X", 
                          &sdata1.Data[0].x, &sdata1.Data[0].y, 
                          sdata1.Data.size(), 
                          -INFINITY, 
                          0,  
                          sdata1.Offset, 
                          2 * sizeof(float));
        

        ImPlot::PlotLine("Mouse Y", 
                        &sdata2.Data[0].x, &sdata2.Data[0].y, 
                        sdata2.Data.size(), 
                        0,  
                        sdata2.Offset, 
                        2 * sizeof(float));
        
        ImPlot::EndPlot();
    }
    

    if (ImPlot::BeginPlot("##Rolling", ImVec2(-1, -1))) {
        ImPlot::SetupAxes(nullptr, nullptr, flags, flags);
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, history, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1);
        

        ImPlot::PlotLine("Mouse X", 
                        &rdata1.Data[0].x, &rdata1.Data[0].y, 
                        rdata1.Data.size(), 
                        0,  
                        0,  
                        2 * sizeof(float));
        
        ImPlot::PlotLine("Mouse Y", 
                        &rdata2.Data[0].x, &rdata2.Data[0].y, 
                        rdata2.Data.size(), 
                        0,  
                        0,  
                        2 * sizeof(float));
        
        ImPlot::EndPlot();
    }
    ImGui::End();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, hInstance, nullptr, nullptr, nullptr, nullptr, _T("ImGui Layout Demo"), nullptr };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("Draggable Split Layout Demo"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, hInstance, nullptr);
    if (!CreateDeviceD3D(hwnd)) { CleanupDeviceD3D(); ::UnregisterClass(wc.lpszClassName, hInstance); return 1; }
    ::ShowWindow(hwnd, nCmdShow); ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION(); 
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io; io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd); ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    

    auto* root = new FrameGUILayout::CustomLayoutNode(false, "Root");


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


    auto* row2 = new FrameGUILayout::CustomLayoutNode(true, "relplot");
    row2->SetVerticalChildren(new FrameGUILayout::CustomLayoutNode(&RealtimePlots, "rel"));

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
    ImPlot::DestroyContext();
    ImGui_ImplDX11_Shutdown(); 
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
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
