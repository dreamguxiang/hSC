#include <windows.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <d3d12.h>

#include "MinHook.h"

#include "uglhook.h"
#include "d3d12hook.h"
#include "globals.h"

namespace D3D12Hooks {
  typedef long (STDMETHODCALLTYPE *PresentFnD3D12)(IDXGISwapChain3 *, UINT, UINT);
  typedef void (STDMETHODCALLTYPE *DrawInstancedFnD3D12)(ID3D12GraphicsCommandList *, UINT, UINT, UINT, UINT);
  typedef void (STDMETHODCALLTYPE *DrawIndexedInstancedFnD3D12)(ID3D12GraphicsCommandList *, UINT, UINT, UINT, INT);
  typedef ULONG (STDMETHODCALLTYPE *ReleaseFnD3D12)(IDXGISwapChain3 *);

  // Trampoline function pointers.
  static PresentFnD3D12 oPresentD3D12;
  static DrawInstancedFnD3D12 oDrawInstancedD3D12;
  static DrawIndexedInstancedFnD3D12 oDrawIndexedInstancedD3D12;
  static ReleaseFnD3D12 oReleaseD3D12;
  static void (*oExecuteCommandListsD3D12)(ID3D12CommandQueue *, UINT, ID3D12CommandList *);
  static HRESULT (*oSignalD3D12)(ID3D12CommandQueue *, ID3D12Fence *, UINT64);

  // Static data.
  static void *hookedFunctions[6] = {0};
  static CRITICAL_SECTION critical = {0};

  // Callbacks.
  static InitCB cbInit = nullptr;
  static PresentCB cbPresent = nullptr;
  static DeinitCB cbDeinit = nullptr;

  // User specified data.
  static void *pUser = nullptr;

  IDXGISwapChain3 *gSavedSwapChain = nullptr;
  ID3D12Device *gDevice = nullptr;
  ID3D12DescriptorHeap *gHeapRTV = nullptr;
  ID3D12DescriptorHeap *gHeapSRV = nullptr;
  ID3D12GraphicsCommandList *gCommandList = nullptr;
  ID3D12Fence *gFence = nullptr;
  UINT64 gFenceValue;
  ID3D12CommandQueue *gCommandQueue = nullptr;
  UINT gBufferCount;
  FrameContext *gFrameContext = nullptr;
  bool gInit = false;

  // Functions.
  bool createAllFrom(IDXGISwapChain3 *pSwapChain) {
    if (!SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D12Device), (void **)&gDevice)))
      return false;

    if (!UniHookGlobals::mainWindow)
      pSwapChain->GetHwnd(&UniHookGlobals::mainWindow);
    if (!UniHookGlobals::mainWindow)
      UniHookGlobals::mainWindow = GetForegroundWindow();

    DXGI_SWAP_CHAIN_DESC sdesc;
    pSwapChain->GetDesc(&sdesc);
    
    gBufferCount = sdesc.BufferCount;
    gFrameContext = new FrameContext[gBufferCount];

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = gBufferCount;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    if (gDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&gHeapSRV)) != S_OK)
      return false;

    ID3D12CommandAllocator *allocator;
    if (gDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)) != S_OK)
      return false;

    for (size_t i = 0; i < gBufferCount; i++)
      gFrameContext[i].commandAllocator = allocator;

    if (
      gDevice->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        allocator,
        NULL,
        IID_PPV_ARGS(&gCommandList)) != S_OK
      || gCommandList->Close() != S_OK
    )
      return false;

    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.NumDescriptors = gBufferCount;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.NodeMask = 1;

    if (gDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&gHeapRTV)) != S_OK)
      return false;

    const auto rtvDescriptorSize = gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = gHeapRTV->GetCPUDescriptorHandleForHeapStart();

    for (size_t i = 0; i < gBufferCount; i++) {
      ID3D12Resource* pBackBuffer = nullptr;

      gFrameContext[i].mainRenderTargetDescriptor = rtvHandle;
      pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
      gDevice->CreateRenderTargetView(pBackBuffer, nullptr, rtvHandle);
      gFrameContext[i].mainRenderTargetResource = pBackBuffer;
      rtvHandle.ptr += rtvDescriptorSize;
    }

    EnterCriticalSection(&critical);
    if (cbInit)
      cbInit(&sdesc, pUser);
    LeaveCriticalSection(&critical);

    return true;
  }

  void clearAll() {
    EnterCriticalSection(&critical);
    if (cbDeinit)
      cbDeinit(pUser);
    LeaveCriticalSection(&critical);

    for (UINT i = 0; i < gBufferCount; i++) {
      SafeRelease(&gFrameContext[i].mainRenderTargetResource);
      SafeRelease(&gFrameContext[i].commandAllocator);
    }

    SafeRelease(&gCommandList);
    SafeRelease(&gHeapRTV);
    SafeRelease(&gHeapSRV);
    SafeRelease(&gDevice);

    gCommandQueue = nullptr;
    gFenceValue = 0;
    gFence = nullptr;
  }

  HRESULT STDMETHODCALLTYPE hookPresentD3D12(
    IDXGISwapChain3 *pSwapChain,
    UINT SyncInterval,
    UINT Flags
  ) {
    if (!gInit) {
      // Initialize from Present.
      gSavedSwapChain = pSwapChain;
      gInit = createAllFrom(pSwapChain);
    }

    if (gInit) {
      if (gCommandQueue == nullptr)
        return oPresentD3D12(pSwapChain, SyncInterval, Flags);

      EnterCriticalSection(&critical);
      if (cbPresent)
        cbPresent(pUser);
      LeaveCriticalSection(&critical);
    }

    return oPresentD3D12(pSwapChain, SyncInterval, Flags);
  }

  void STDMETHODCALLTYPE hookkDrawInstancedD3D12(
    ID3D12GraphicsCommandList* dCommandList,
    UINT VertexCountPerInstance,
    UINT InstanceCount,
    UINT StartVertexLocation,
    UINT StartInstanceLocation
  ) {
    return oDrawInstancedD3D12(
      dCommandList,
      VertexCountPerInstance,
      InstanceCount,
      StartVertexLocation,
      StartInstanceLocation);
  }

  void STDMETHODCALLTYPE hookDrawIndexedInstancedD3D12(
    ID3D12GraphicsCommandList* dCommandList,
    UINT IndexCount,
    UINT InstanceCount,
    UINT StartIndex,
    INT BaseVertex
  ) {
    return oDrawIndexedInstancedD3D12(
      dCommandList,
      IndexCount,
      InstanceCount,
      StartIndex,
      BaseVertex);
  }

  void STDMETHODCALLTYPE hookExecuteCommandListsD3D12(
    ID3D12CommandQueue *queue,
    UINT NumCommandLists,
    ID3D12CommandList *ppCommandLists
  ) {
    if (!gCommandQueue)
      gCommandQueue = queue;

    oExecuteCommandListsD3D12(queue, NumCommandLists, ppCommandLists);
  }

  HRESULT STDMETHODCALLTYPE hookSignalD3D12(
    ID3D12CommandQueue* queue,
    ID3D12Fence* fence,
    UINT64 value
  ) {
    if (gCommandQueue != nullptr && queue == gCommandQueue) {
      gFence = fence;
      gFenceValue = value;
    }

    return oSignalD3D12(queue, fence, value);
  }

  ULONG hookReleaseD3D12(IDXGISwapChain3 *pSwapChain) {
    if (gInit && pSwapChain == gSavedSwapChain) {
      gInit = false;
      clearAll();
    }
    return oReleaseD3D12(pSwapChain);
  }

  bool init(InitCB init, PresentCB present, DeinitCB deinit, void *lpUser) {
    HWND hookedWnd;
    HMODULE dxgiDll
      , d3d12Dll;
    FARPROC fnCreateDXGIFactory
      , fnD3D12CreateDevice;
    IDXGIFactory *factory;
    IDXGIAdapter *adapter;
    ID3D12Device *device;
    ID3D12CommandQueue *commandQueue;
    ID3D12CommandAllocator *commandAllocator;
    ID3D12GraphicsCommandList *commandList;
    IDXGISwapChain *swapChain;

    WNDCLASSEXA windowClass;
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = DefWindowProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = GetModuleHandle(NULL);
    windowClass.hIcon = NULL;
    windowClass.hCursor = NULL;
    windowClass.hbrBackground = NULL;
    windowClass.lpszMenuName = NULL;
    windowClass.lpszClassName = "UGLHookWnd";
    windowClass.hIconSm = NULL;

    ::InitializeCriticalSection(&critical);

    if (!::RegisterClassExA(&windowClass) && ::GetLastError() != ERROR_ALREADY_EXISTS)
      return false;

    hookedWnd = ::CreateWindowA(
      windowClass.lpszClassName,
      "UGLH_DX12",
      WS_OVERLAPPEDWINDOW,
      0, 0, 100, 100,
      nullptr, nullptr,
      windowClass.hInstance,
      nullptr);
    
    if (!hookedWnd)
      return false;

    dxgiDll = ::GetModuleHandleA("dxgi.dll");
    d3d12Dll = ::GetModuleHandleA("d3d12.dll");
    if (!dxgiDll || !d3d12Dll) {
      ::DestroyWindow(hookedWnd);
      ::UnregisterClassA(windowClass.lpszClassName, windowClass.hInstance);
      return false;
    }

    fnCreateDXGIFactory = ::GetProcAddress(dxgiDll, "CreateDXGIFactory");
    if (!fnCreateDXGIFactory) {
      ::DestroyWindow(hookedWnd);
      ::UnregisterClassA(windowClass.lpszClassName, windowClass.hInstance);
      return false;
    }

    if (((long (__stdcall *)(const IID&, void **))(fnCreateDXGIFactory))(__uuidof(IDXGIFactory), (void**)&factory) < 0) {
      ::DestroyWindow(hookedWnd);
      ::UnregisterClassA(windowClass.lpszClassName, windowClass.hInstance);
      return false;
    }

    if (factory->EnumAdapters(0, &adapter) == DXGI_ERROR_NOT_FOUND) {
      ::DestroyWindow(hookedWnd);
      ::UnregisterClassA(windowClass.lpszClassName, windowClass.hInstance);
      return false;
    }

    fnD3D12CreateDevice = ::GetProcAddress(d3d12Dll, "D3D12CreateDevice");
    if (!fnD3D12CreateDevice) {
      ::DestroyWindow(hookedWnd);
      ::UnregisterClassA(windowClass.lpszClassName, windowClass.hInstance);
      return false;
    }

    if (((long (__stdcall *)(IUnknown *, D3D_FEATURE_LEVEL, const IID&, void **))(fnD3D12CreateDevice))(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&device) < 0) {
      ::DestroyWindow(hookedWnd);
      ::UnregisterClassA(windowClass.lpszClassName, windowClass.hInstance);
      return false;
    }

    D3D12_COMMAND_QUEUE_DESC queueDesc;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Priority = 0;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;

    if (device->CreateCommandQueue(&queueDesc, __uuidof(ID3D12CommandQueue), (void **)&commandQueue) < 0) {
      ::DestroyWindow(hookedWnd);
      ::UnregisterClassA(windowClass.lpszClassName, windowClass.hInstance);
      return false;
    }

    if (device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void **)&commandAllocator) < 0) {
      ::DestroyWindow(hookedWnd);
      ::UnregisterClassA(windowClass.lpszClassName, windowClass.hInstance);
      return false;
    }

    if (device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, __uuidof(ID3D12GraphicsCommandList), (void **)&commandList) < 0) {
      ::DestroyWindow(hookedWnd);
      ::UnregisterClassA(windowClass.lpszClassName, windowClass.hInstance);
      return false;
    }

    DXGI_RATIONAL refreshRate;
    refreshRate.Numerator = 60;
    refreshRate.Denominator = 1;

    DXGI_MODE_DESC bufferDesc;
    bufferDesc.Width = 100;
    bufferDesc.Height = 100;
    bufferDesc.RefreshRate = refreshRate;
    bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    DXGI_SAMPLE_DESC sampleDesc;
    sampleDesc.Count = 1;
    sampleDesc.Quality = 0;

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferDesc = bufferDesc;
    swapChainDesc.SampleDesc = sampleDesc;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.OutputWindow = hookedWnd;
    swapChainDesc.Windowed = 1;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    if (factory->CreateSwapChain(commandQueue, &swapChainDesc, &swapChain) < 0) {
      ::DestroyWindow(hookedWnd);
      ::UnregisterClassA(windowClass.lpszClassName, windowClass.hInstance);
      return false;
    }

    EnterCriticalSection(&critical);
    cbInit = init;
    cbPresent = present;
    cbDeinit = deinit;
    pUser = lpUser;
    LeaveCriticalSection(&critical);

    hookedFunctions[0] = (*(void ***)commandQueue)[10];
    hookedFunctions[1] = (*(void ***)commandQueue)[14];
    hookedFunctions[2] = (*(void ***)commandList)[12];
    hookedFunctions[3] = (*(void ***)commandList)[13];
    hookedFunctions[4] = (*(void ***)swapChain)[2];
    hookedFunctions[5] = (*(void ***)swapChain)[8];

    MH_Initialize();
    MH_CreateHook(hookedFunctions[0], (void *)hookExecuteCommandListsD3D12, (void **)&oExecuteCommandListsD3D12);
    MH_CreateHook(hookedFunctions[1], (void *)hookSignalD3D12, (void **)&oSignalD3D12);
    MH_CreateHook(hookedFunctions[2], (void *)hookkDrawInstancedD3D12, (void **)&oDrawInstancedD3D12);
    MH_CreateHook(hookedFunctions[3], (void *)hookDrawIndexedInstancedD3D12, (void **)&oDrawIndexedInstancedD3D12);
    MH_CreateHook(hookedFunctions[4], (void *)hookReleaseD3D12, (void **)&oReleaseD3D12);
    MH_CreateHook(hookedFunctions[5], (void *)hookPresentD3D12, (void **)&oPresentD3D12);

    for (int i = 0; i < 6; i++)
      MH_EnableHook(hookedFunctions[i]);

    device->Release();
    device = nullptr;

    commandQueue->Release();
    commandQueue = nullptr;

    commandAllocator->Release();
    commandAllocator = nullptr;

    commandList->Release();
    commandList = nullptr;

    swapChain->Release();
    swapChain = nullptr;

    ::DestroyWindow(hookedWnd);
    ::UnregisterClassA(windowClass.lpszClassName, windowClass.hInstance);

    return true;
  }

  bool deinit() {
    clearAll();
    for (int i = 0; i < 6; i++) {
      MH_DisableHook(hookedFunctions[i]);
      hookedFunctions[i] = nullptr;
    }
    EnterCriticalSection(&critical);
    cbInit = nullptr;
    cbPresent = nullptr;
    cbDeinit = nullptr;
    pUser = nullptr;
    LeaveCriticalSection(&critical);
    return true;
  }
}
