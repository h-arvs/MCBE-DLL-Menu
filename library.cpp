#include <print>

#include <d3d11.h>
#include <d3d11on12.h>
#include <d3d12.h>
#include <deque>
#include <dxgi1_4.h>
#include <kiero.hpp>
#include <safetyhook.hpp>
#include <windows.h>
#include <winrt/base.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"

#include "GameInput/GameInput.h"

HWND window{};
bool uninject = false;

winrt::com_ptr<ID3D11Device> g_d3d11Device{};
winrt::com_ptr<ID3D11On12Device> g_d3d11on12Device{};
winrt::com_ptr<ID3D11DeviceContext> g_d3d11DeviceContext{};
winrt::com_ptr<ID3D12CommandQueue> g_d3d12CommandQueue{};

struct BufferData {
    winrt::com_ptr<ID3D12Resource> native;
    winrt::com_ptr<ID3D11Resource> wrapped;
};

static std::vector<BufferData> wrappedBuffers;

safetyhook::InlineHook presentHookImpl{};

HRESULT presentHook(IDXGISwapChain3 *pSwapChain, UINT SyncInterval, UINT Flags) {
    if (!g_d3d11Device) {
        winrt::com_ptr<ID3D12Device> d3d12Device{};
        if (SUCCEEDED(pSwapChain->GetDevice(IID_PPV_ARGS(d3d12Device.put())))) {
            UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_SINGLETHREADED;

            auto commandQueue = g_d3d12CommandQueue.get();

            if (!commandQueue) {
                return presentHookImpl.call<HRESULT>(pSwapChain, SyncInterval, Flags);
            }

            winrt::check_hresult(D3D11On12CreateDevice(d3d12Device.get(), deviceFlags, nullptr, 0,
                                                       reinterpret_cast<IUnknown *const *>(&commandQueue), 1, 0,
                                                       g_d3d11Device.put(), g_d3d11DeviceContext.put(), nullptr));

            g_d3d11on12Device = g_d3d11Device.as<ID3D11On12Device>();

            std::println("Initialized d3d11on12");
        } else if (SUCCEEDED(pSwapChain->GetDevice(IID_PPV_ARGS(g_d3d11Device.put())))) {
            g_d3d11Device->GetImmediateContext(g_d3d11DeviceContext.put());
        } else {
            return presentHookImpl.call<HRESULT>(pSwapChain, SyncInterval, Flags);
        }

        ImGui::CreateContext();
        ImGui::GetIO().IniFilename = nullptr;
        if (!ImGui_ImplDX11_Init(g_d3d11Device.get(), g_d3d11DeviceContext.get()) || !ImGui_ImplWin32_Init(window)) {
            std::println("Failed to initialize ImGui backends");
            return presentHookImpl.call<HRESULT>(pSwapChain, SyncInterval, Flags);
        }
        std::println("Initialized ImGui");
    }


    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();

    ImGui::Begin("Hello world!");
    if (ImGui::Button("UnInject")) {
        uninject = true;
    }
    ImGui::End();

    ImGui::Render();

    winrt::com_ptr<ID3D11Resource> backBuffer{};
    if (g_d3d11on12Device) {
        DXGI_SWAP_CHAIN_DESC desc;
        winrt::check_hresult(pSwapChain->GetDesc(&desc));
        UINT buffer_count = desc.BufferCount;
        if (buffer_count != wrappedBuffers.size()) {
            wrappedBuffers.clear();
            wrappedBuffers.resize(buffer_count);
            for (UINT i = 0; i < buffer_count; i++) {
                auto &bufferData = wrappedBuffers.at(i);

                winrt::check_hresult(pSwapChain->GetBuffer(i, IID_PPV_ARGS(bufferData.native.put())));

                D3D11_RESOURCE_FLAGS resourceFlags{};
                resourceFlags.BindFlags = D3D11_BIND_RENDER_TARGET;
                winrt::check_hresult(g_d3d11on12Device->CreateWrappedResource(
                        bufferData.native.get(), &resourceFlags, D3D12_RESOURCE_STATE_RENDER_TARGET,
                        D3D12_RESOURCE_STATE_PRESENT, IID_PPV_ARGS(bufferData.wrapped.put())));
            }
            std::println("Wrapped resources");
        }

        backBuffer.copy_from(wrappedBuffers[pSwapChain->GetCurrentBackBufferIndex()].wrapped.get());
        auto backBufferPtr = backBuffer.get();
        g_d3d11on12Device->AcquireWrappedResources(&backBufferPtr, 1);
    } else {
        pSwapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.put()));
    }

    winrt::com_ptr<ID3D11RenderTargetView> g_mainRenderTargetView{};
    winrt::check_hresult(
            g_d3d11Device->CreateRenderTargetView(backBuffer.get(), nullptr, g_mainRenderTargetView.put()));

    auto renderTargetView = g_mainRenderTargetView.get();
    g_d3d11DeviceContext->OMSetRenderTargets(1, &renderTargetView, nullptr);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    if (g_d3d11on12Device) {
        auto backBufferPtr = backBuffer.get();
        g_d3d11on12Device->ReleaseWrappedResources(&backBufferPtr, 1);
    }

    g_d3d11DeviceContext->Flush();

    return presentHookImpl.call<HRESULT>(pSwapChain, SyncInterval, Flags);
};

safetyhook::InlineHook executeCommandListsHookImpl{};

void executeCommandListsHook(ID3D12CommandQueue *queue, UINT NumCommandLists,
                             ID3D12CommandList *const *ppCommandLists) {

    if (!g_d3d12CommandQueue) {
        g_d3d12CommandQueue.copy_from(queue);
        std::println("Got Command Queue");
    }

    return executeCommandListsHookImpl.call<void>(queue, NumCommandLists, ppCommandLists);
};

safetyhook::InlineHook resizeBuffers1HookImpl{};
HRESULT resizeBuffers1Hook(IDXGISwapChain *swapChain, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat,
                           UINT swapChainFlags, const UINT *pCreationNodeMask, IUnknown *ppPresentQueue) {

    if (g_d3d11DeviceContext) {
        if (g_d3d11on12Device) {
            wrappedBuffers.resize(0);
        }

        g_d3d11DeviceContext->Flush();
    }

    return resizeBuffers1HookImpl.call<HRESULT>(swapChain, bufferCount, width, height, newFormat, swapChainFlags,
                                                pCreationNodeMask, ppPresentQueue);
}

safetyhook::InlineHook getMouseStateHookImpl{};
bool getMouseStateHook(GameInput::v2::IGameInputReading *_self, GameInput::v2::GameInputMouseState *mouseState) {
    auto result = getMouseStateHookImpl.call<bool>(_self, mouseState);

    if (result) {
        /*
         * Here you can intercept mouse state before the game uses it.
         * Mouse input messages are still passed through WndProc, so we do not have to poll ImGui mouse events here.
         * However, these messages are not handled in WndProc, so we have to intercept mouse state with
         * this hook.
         */
    }

    return result;
};

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LONG_PTR wndProcO;
LRESULT wndProcHook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return true;

    // You can intercept keyboard input, and all other WndProc messages except mouse, here.

    return CallWindowProc(reinterpret_cast<WNDPROC>(wndProcO), hWnd, uMsg, wParam, lParam);
};

void load(HMODULE module) {
    AllocConsole();
    FILE *fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    std::println("Hi!");

    window = FindWindowA("Bedrock", "Minecraft");

    if (!window) {
        std::println("Failed to get window");
    }


    /*
     * Rather than using WndProc, GDK Minecraft Bedrock uses GameInput to read mouse state.
     * To intercept mouse states before the game reads them, we hook the IGameInputReading::GetMouseState
     * virtual.
     * To get this function at runtime, without any signatures, we create an IGameInput instance with
     * GameInputCreate (exported by GameInput.dll loaded by Minecraft), call GetCurrentReading to get an instance of
     * IGameInputReading, then index into the vtable of this instance to resolve GetMouseState for hooking.
     */

    {
        winrt::com_ptr<GameInput::v2::IGameInput> gameInput{};
        winrt::check_hresult(GameInput::v2::GameInputCreate(gameInput.put()));

        winrt::com_ptr<GameInput::v2::IGameInputReading> gameInputReading{};
        winrt::check_hresult(gameInput->GetCurrentReading(GameInput::v2::GameInputKind::GameInputKindMouse, nullptr,
                                                          gameInputReading.put()));

        constexpr auto vtableIndex =
                kiero::detail::magic_vft::vtable_index<&GameInput::v2::IGameInputReading::GetMouseState>();

        auto vtable = *reinterpret_cast<uintptr_t **>(gameInputReading.get());
        auto function = vtable[vtableIndex];

        getMouseStateHookImpl = safetyhook::create_inline(function, &getMouseStateHook);
    } // This is in its own scope so the com_ptrs are freed at this point rather than at the end of function scope


    /*
     * Though the game uses GameInput for mouse input, it still uses WndProc for keyboard input.
     * We can replace the game's WndProc function, without a signature, using SetWindowLongPtrA.
     */
    wndProcO = SetWindowLongPtrA(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&wndProcHook));
    std::println("Hooked WndProc");

    if (kiero::init(kiero::RenderType::Auto) == kiero::Status::Success) {
        auto renderType = kiero::getRenderType();

        if (renderType == kiero::RenderType::D3D12) {
            std::println("D3D12 Detected");

            executeCommandListsHookImpl = safetyhook::create_inline(
                    kiero::getMethod<&ID3D12CommandQueue::ExecuteCommandLists>(), &executeCommandListsHook);
            std::println("Hooked executeCommandLists");
        } else {
            std::println("D3D11 Detected");
        }

        presentHookImpl = safetyhook::create_inline(kiero::getMethod<&IDXGISwapChain::Present>(), &presentHook);
        std::println("Hooked present");


        resizeBuffers1HookImpl =
                safetyhook::create_inline(kiero::getMethod<&IDXGISwapChain3::ResizeBuffers1>(), &resizeBuffers1Hook);
        std::println("Hooked resizeBuffers1");
    } else {
        std::println("Failed to initialize Kiero");
    }

    while (!uninject)
        std::this_thread::sleep_for(std::chrono::milliseconds(19));
    // UnHook everything
    presentHookImpl.reset();
    executeCommandListsHookImpl.reset();
    resizeBuffers1HookImpl.reset();
    getMouseStateHookImpl.reset();
    SetWindowLongPtrA(window, GWLP_WNDPROC, wndProcO);

    std::println("Unhooked all hooks");

    wrappedBuffers.resize(0); // Free wrapped resources that we allocated
    g_d3d11DeviceContext->Flush();

    std::this_thread::sleep_for(std::chrono::seconds(1)); // Make sure hooks return safely


    std::println("UnInjected");
    FreeConsole();
    FreeLibraryAndExitThread(module, 0); // All our references get released because we use com_ptr in file scope!!
}

bool WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpRes) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstance);
            CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(load), hInstance, 0, nullptr);
            break;
        default:;
    };

    return TRUE;
};
