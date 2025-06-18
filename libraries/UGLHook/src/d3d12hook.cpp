#include <windows.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <d3d12.h>

#include "d3d12hook.h"
#include "globals.h"
#include "kiero.h"

namespace D3D12Hooks {
  typedef long (STDMETHODCALLTYPE *PresentFnD3D12)(IDXGISwapChain *, UINT, UINT);
  typedef void (STDMETHODCALLTYPE *DrawInstancedFnD3D12)(ID3D12GraphicsCommandList *, UINT, UINT, UINT, UINT);
  typedef void (STDMETHODCALLTYPE *DrawIndexedInstancedFnD3D12)(ID3D12GraphicsCommandList *, UINT, UINT, UINT, INT);
  typedef ULONG (STDMETHODCALLTYPE *ReleaseFnD3D12)(IDXGISwapChain3 *);

  // Trampoline function pointers.
  static PresentFnD3D12 oPresentD3D12;
  static DrawInstancedFnD3D12 oDrawInstancedD3D12;
  static DrawIndexedInstancedFnD3D12 oDrawIndexedInstancedD3D12;
  static ReleaseFnD3D12 oReleaseD3D12;
  static void (*oExecuteCommandListsD3D12)(ID3D12CommandQueue*, UINT, ID3D12CommandList*);
  static HRESULT (*oSignalD3D12)(ID3D12CommandQueue*, ID3D12Fence*, UINT64);

  // Callbacks.
  static InitCB cbInit = nullptr;
  static PresentCB cbPresent = nullptr;
  static DeinitCB cbDeinit = nullptr;

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

  static void *pUser = nullptr;

  // Functions.
  bool createAllFrom(IDXGISwapChain3* pSwapChain) {
    if (!SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D12Device), (void**)&gDevice)))
      return false;

    if (!UniHookGlobals::mainWindow)
      pSwapChain->GetHwnd(&UniHookGlobals::mainWindow);
    if (!UniHookGlobals::mainWindow)
      UniHookGlobals::mainWindow = GetForegroundWindow();

    DXGI_SWAP_CHAIN_DESC sdesc;
    pSwapChain->GetDesc(&sdesc);
    //sdesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    //sdesc.OutputWindow = UniHookGlobals::mainWindow;
    //sdesc.Windowed = ((GetWindowLongPtr(UniHookGlobals::mainWindow, GWL_STYLE) & WS_POPUP) != 0) ? false : true;

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
      gDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, NULL, IID_PPV_ARGS(&gCommandList)) != S_OK
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

    if (cbInit)
      cbInit(pUser, &sdesc);

    return true;
  }

  void clearAll() {
    if (cbDeinit)
      cbDeinit(pUser);

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
    IDXGISwapChain3* pSwapChain,
    UINT SyncInterval,
    UINT Flags
  ) {
    /*if (GetAsyncKeyState(UniHookGlobals::openMenuKey) & 0x1)
      menu::isOpen ? menu::isOpen = false : menu::isOpen = true;*/

    if (!gInit) {
      // Initialize from Present.
      gSavedSwapChain = pSwapChain;
      gInit = createAllFrom(pSwapChain);
    }

    if (gInit) {
      if (gCommandQueue == nullptr)
        return oPresentD3D12(pSwapChain, SyncInterval, Flags);

      if (cbPresent)
        cbPresent(pUser);
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
    ID3D12CommandQueue* queue,
    UINT NumCommandLists,
    ID3D12CommandList* ppCommandLists
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
    if (kiero::init(kiero::RenderType::D3D12) != kiero::Status::Success)
      return false;
    
    cbInit = init;
    cbPresent = present;
    cbDeinit = deinit;
    pUser = lpUser;

    kiero::bind(54, (void**)&oExecuteCommandListsD3D12, (void *)hookExecuteCommandListsD3D12);
    kiero::bind(58, (void**)&oSignalD3D12, (void *)hookSignalD3D12);
    kiero::bind(84, (void**)&oDrawInstancedD3D12, (void *)hookkDrawInstancedD3D12);
    kiero::bind(85, (void**)&oDrawIndexedInstancedD3D12, (void *)hookDrawIndexedInstancedD3D12);
    kiero::bind(134, (void **)&oReleaseD3D12, (void *)hookReleaseD3D12);
    kiero::bind(140, (void**)&oPresentD3D12, (void *)hookPresentD3D12);

    return true;
  }

  bool deinit() {
    clearAll();
    kiero::shutdown();
    //InputHandler::Remove(UniHookGlobals::mainWindow);
    return true;
  }
}
