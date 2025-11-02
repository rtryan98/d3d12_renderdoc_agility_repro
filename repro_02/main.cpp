#include <../include/d3d12.h>
#include <array>
#include <cstdint>
#include <dxgi1_6.h>
#include <exception>
#include <fstream>
#include <vector>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

extern "C" __declspec(dllexport) const uint32_t D3D12SDKVersion = 618;
extern "C" __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";

ID3D12Device* device_ref;
void throw_if_failed(HRESULT hr)
{
    if (device_ref)
    {
        auto device_removal_hr = device_ref->GetDeviceRemovedReason();
        if (FAILED(device_removal_hr))
        {
            printf("Device Removal HRESULT: %u\n", device_removal_hr);
        }
    }
    if (FAILED(hr))
    {
        throw std::exception();
    }
}

DWORD d3d12_queue_wait_idle(ID3D12Device* device, ID3D12CommandQueue* queue)
{
    DWORD result = WAIT_FAILED;
    ComPtr<ID3D12Fence> fence;
    throw_if_failed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    throw_if_failed(queue->Signal(fence.Get(), 1));
    if (fence->GetCompletedValue() < 1)
    {
        HANDLE event_handle = CreateEvent(NULL, FALSE, FALSE, NULL);
        throw_if_failed(fence->SetEventOnCompletion(1, event_handle));
        if (event_handle != 0)
        {
            result = WaitForSingleObject(event_handle, INFINITE);
            CloseHandle(event_handle);
        }
    }
    else
    {
        result = WAIT_OBJECT_0;
    }
    return result;
}

static bool window_alive = false;
static uint32_t window_width = 0;
static uint32_t window_height = 0;

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_CLOSE:
        window_alive = false;
        break;
    case WM_GETMINMAXINFO:
    {
        auto& size = reinterpret_cast<LPMINMAXINFO>(lparam)->ptMinTrackSize;
        size.x = 256;
        size.y = 144;
        break;
    }
    case WM_SIZE:
    {
        RECT rect;
        GetClientRect(hwnd, &rect);
        window_width = rect.right;
        window_height = rect.bottom;
        break;
    }
    default:
        break;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

HWND create_window(uint32_t width, uint32_t height, const char* title)
{
    HWND result = {};
    RECT wr = {
        .left = LONG((GetSystemMetrics(SM_CXSCREEN) - width) / 2),
        .top = LONG((GetSystemMetrics(SM_CYSCREEN) - height) / 2),
        .right = LONG(width),
        .bottom = LONG(height)
    };
    DWORD style = WS_OVERLAPPEDWINDOW;
    AdjustWindowRectEx(&wr, style, FALSE, WS_EX_TOOLWINDOW);
    WNDCLASSEX wc = {
        .cbSize = sizeof(WNDCLASSEX),
        .style = 0,
        .lpfnWndProc = wnd_proc,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = GetModuleHandle(NULL),
        .hIcon = LoadIcon(NULL, IDI_WINLOGO),
        .hCursor = LoadCursor(NULL, IDC_ARROW),
        .hbrBackground = HBRUSH(GetStockObject(BLACK_BRUSH)),
        .lpszMenuName = nullptr,
        .lpszClassName = title,
        .hIconSm = wc.hIcon
    };
    RegisterClassEx(&wc);
    result = CreateWindowEx(
        0,
        wc.lpszClassName,
        wc.lpszClassName,
        style,
        wr.left,
        wr.top,
        wr.right,
        wr.bottom,
        nullptr,
        nullptr,
        GetModuleHandle(NULL),
        0);
    ShowWindow(result, SW_SHOWDEFAULT);
    SetForegroundWindow(result);
    SetFocus(result);
    window_alive = true;
    return result;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    static constexpr uint32_t MAX_SWAPCHAIN_BUFFERS = 2;

    HWND window = create_window(1280, 720, "D3D12 Renderdoc Crash Repro");
    ComPtr<IDXGIFactory7> factory;
    throw_if_failed(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));
    ComPtr<IDXGIAdapter4> adapter;
    throw_if_failed(factory->EnumAdapterByGpuPreference(
        0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)));
    ComPtr<ID3D12Debug6> debug;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
    {
        debug->EnableDebugLayer();
    }
    ComPtr<ID3D12Device12> device;
    throw_if_failed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device)));
    device_ref = device.Get();
    ComPtr<ID3D12CommandQueue> queue;
    D3D12_COMMAND_QUEUE_DESC queue_desc = {
        .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
        .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
        .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
        .NodeMask = 0
    };
    throw_if_failed(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queue)));
    ComPtr<ID3D12DescriptorHeap> rtv_descriptor_heap;
    D3D12_DESCRIPTOR_HEAP_DESC rtv_descriptor_heap_desc = {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        .NumDescriptors = MAX_SWAPCHAIN_BUFFERS,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        .NodeMask = 0
    };
    throw_if_failed(device->CreateDescriptorHeap(&rtv_descriptor_heap_desc, IID_PPV_ARGS(&rtv_descriptor_heap)));
    ComPtr<IDXGISwapChain1> swapchain1;
    DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = {.Count = 1, .Quality = 0 },
        .BufferCount = MAX_SWAPCHAIN_BUFFERS,
        .Scaling = DXGI_SCALING_STRETCH,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
        .Flags = 0
    };
    throw_if_failed(factory->CreateSwapChainForHwnd(queue.Get(), window, &swapchain_desc, nullptr, nullptr, &swapchain1));
    ComPtr<IDXGISwapChain4> swapchain;
    throw_if_failed(swapchain1->QueryInterface(IID_PPV_ARGS(&swapchain)));
    std::array<ComPtr<ID3D12Resource>, MAX_SWAPCHAIN_BUFFERS> swapchain_buffers = {};
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, MAX_SWAPCHAIN_BUFFERS> swapchain_descriptors = {};
    for (uint32_t i = 0; i < MAX_SWAPCHAIN_BUFFERS; ++i)
    {
        swapchain->GetBuffer(i, IID_PPV_ARGS(&swapchain_buffers[i]));
        auto descriptor_handle = rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
        auto increment = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        descriptor_handle.ptr += i * increment;
        D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
            .Texture2D = {
                .MipSlice = 0,
                .PlaneSlice = 0
            }
        };
        device->CreateRenderTargetView(swapchain_buffers[i].Get(), &rtv_desc, descriptor_handle);
        swapchain_descriptors[i] = descriptor_handle;
    }

    ComPtr<ID3D12CommandAllocator> cmd_allocator;
    throw_if_failed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmd_allocator)));
    ComPtr<ID3D12GraphicsCommandList7> cmd;
    throw_if_failed(device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&cmd)));


    // START RELEVANT SECTION

    ComPtr<ID3D12Heap1> heap;
    ComPtr<ID3D12Resource> gpu_resource;
    ComPtr<ID3D12Resource> readback_resource;

    D3D12_RESOURCE_DESC1 resource_desc = {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = 0,
        .Width = 256,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = {.Count = 1, .Quality = 0 },
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = D3D12_RESOURCE_FLAG_USE_TIGHT_ALIGNMENT,
        .SamplerFeedbackMipRegion = {}
    };
    D3D12_HEAP_PROPERTIES heap_props = {
        .Type = D3D12_HEAP_TYPE_DEFAULT,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask = 0,
        .VisibleNodeMask = 0
    };
    throw_if_failed(device->CreateCommittedResource3(
        &heap_props, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_BARRIER_LAYOUT_UNDEFINED,
        nullptr, nullptr, 0, nullptr, IID_PPV_ARGS(&gpu_resource)));
    heap_props = {
        .Type = D3D12_HEAP_TYPE_READBACK,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask = 0,
        .VisibleNodeMask = 0
    };
    D3D12_HEAP_DESC heap_desc = {
        .SizeInBytes = 4096,
        .Properties = heap_props,
        .Alignment = 0,
        .Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS
    };
    throw_if_failed(device->CreateHeap1(
        &heap_desc, nullptr, IID_PPV_ARGS(&heap)));
    throw_if_failed(device->CreatePlacedResource2(
        heap.Get(), 0, &resource_desc, D3D12_BARRIER_LAYOUT_UNDEFINED, nullptr, 0, nullptr, IID_PPV_ARGS(&readback_resource)));

    // END RELEVANT SECTION


    while (window_alive)
    {
        MSG msg;
        ZeroMemory(&msg, sizeof(MSG));
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        d3d12_queue_wait_idle(device.Get(), queue.Get());
        cmd_allocator->Reset();
        cmd->Reset(cmd_allocator.Get(), nullptr);


        // START RELEVANT SECTION
        static uint64_t frame = 0;

        cmd->CopyBufferRegion(readback_resource.Get(), 0, gpu_resource.Get(), 128 * (frame % 2), 128);

        frame += 1;
        // END RELEVANT SECTION


        auto current_image = swapchain->GetCurrentBackBufferIndex();
        D3D12_TEXTURE_BARRIER scbar = {
            .SyncBefore = D3D12_BARRIER_SYNC_NONE,
            .SyncAfter = D3D12_BARRIER_SYNC_RENDER_TARGET,
            .AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS,
            .AccessAfter = D3D12_BARRIER_ACCESS_RENDER_TARGET,
            .LayoutBefore = D3D12_BARRIER_LAYOUT_UNDEFINED,
            .LayoutAfter = D3D12_BARRIER_LAYOUT_RENDER_TARGET,
            .pResource = swapchain_buffers[current_image].Get(),
            .Subresources = {
                .IndexOrFirstMipLevel = 0,
                .NumMipLevels = 1,
                .FirstArraySlice = 0,
                .NumArraySlices = 1,
                .FirstPlane = 0,
                .NumPlanes = 1
            },
            .Flags = D3D12_TEXTURE_BARRIER_FLAG_NONE
        };
        D3D12_BARRIER_GROUP scbargrp = {
            .Type = D3D12_BARRIER_TYPE_TEXTURE,
            .NumBarriers = 1,
            .pTextureBarriers = &scbar
        };
        cmd->Barrier(1, &scbargrp);
        float cc[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
        cmd->ClearRenderTargetView(swapchain_descriptors[current_image], cc, 0, nullptr);
        scbar.SyncBefore = scbar.SyncAfter;
        scbar.SyncAfter = D3D12_BARRIER_SYNC_NONE;
        scbar.AccessBefore = scbar.AccessAfter;
        scbar.AccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS;
        scbar.LayoutBefore = scbar.LayoutAfter;
        scbar.LayoutAfter = D3D12_BARRIER_LAYOUT_PRESENT;
        cmd->Barrier(1, &scbargrp);
        cmd->Close();
        ID3D12CommandList* submitcmd = cmd.Get();
        queue->ExecuteCommandLists(1, &submitcmd);
        swapchain->Present(0, 0);
    }
    d3d12_queue_wait_idle(device.Get(), queue.Get());
    return 0;
}
