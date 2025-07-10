#ifndef __UGLH_D3D12HOOK_H__
#define __UGLH_D3D12HOOK_H__

#include <windows.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <d3d12.h>

#include "aliases.h"

namespace D3D12Hooks {
  typedef void (__fastcall *InitCB)(const DXGI_SWAP_CHAIN_DESC *, void *);
  typedef void (__fastcall *PresentCB)(void *);
  typedef void (__fastcall *DeinitCB)(void *);

  struct FrameContext {
    ID3D12CommandAllocator* commandAllocator = nullptr;
    ID3D12Resource* mainRenderTargetResource = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE mainRenderTargetDescriptor;
  };

  extern IDXGISwapChain3 *gSavedSwapChain;
  extern ID3D12Device *gDevice;
  extern ID3D12DescriptorHeap *gHeapRTV;
  extern ID3D12DescriptorHeap *gHeapSRV;
  extern ID3D12GraphicsCommandList *gCommandList;
  extern ID3D12Fence *gFence;
  extern UINT64 gFenceValue;
  extern ID3D12CommandQueue* gCommandQueue;
  extern UINT gBufferCount;
  extern FrameContext *gFrameContext;
  extern bool gInit;

  /**
   * Install hooks with given callback functions.
   * 
   * Workflow: 
   * Inject and install hooks.
   * -> Wait for the first call of IDXGISwapChain::Present.
   * -> Get all dx12 data and call InitCB (Initialize overlay renderer).
   * -> Call PresentCB every time IDXGISwapChain::Present is called when the
   *    initialize is done (Render overlay).
   * -> Call DeinitCB when IDXGISwapChain::Release is called (Destroy overlay
   *    renderer and wait for next possible reinitialize).
   * 
   * The resize event is automatically processed.
   * 
   * NOTE: The function `init` may be called multiple times due to possible
   * release operations on the swap chain of the injected process. DO NOT 
   * include operations within it that must be executed only once.
   * 
   * @param init Callback of the initialize of device and swap chain.
   * @param present Function to be call when the Present function is called.
   * @param deinit Function to be call when the Release function is called.
   * @param lpUser User data pointer.
   */
  bool init(InitCB init, PresentCB present, DeinitCB deinit, void *lpUser);

  /**
   * Remove all hooks and release dx12 objects created.
   */
  bool deinit();
}

template<typename T> void SafeRelease(T **obj) {
  if (*obj) {
    (*obj)->Release();
    *obj = nullptr;
  }
}

#endif
