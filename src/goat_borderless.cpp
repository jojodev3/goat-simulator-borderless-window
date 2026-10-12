// Goat Simulator (UE3, D3D9, 32-bit) borderless window mod.
// dinput8.dll proxy + MinHook. Based on the technique used by
// lanylow/outlast-borderless-window (GPL-3.0) -> this port is GPL-3.0 as well.
//
// When the game asks Direct3D 9 for a fullscreen device, we change the request
// to windowed, strip the window frame, and stretch the window over the monitor.

#include <windows.h>
#include <d3d9.h>
#include <cstdint>
#include "MinHook.h"

// ---------------------------------------------------------------- dinput8 proxy
typedef HRESULT (WINAPI *DirectInput8Create_t)(HINSTANCE, DWORD, const void*, LPVOID*, LPUNKNOWN);
static HMODULE g_real_dinput8 = nullptr;

extern "C" HRESULT WINAPI DirectInput8Create_Proxy(HINSTANCE inst, DWORD ver, const void* riid, LPVOID* out, LPUNKNOWN outer) {
  if (!g_real_dinput8) {
    char path[MAX_PATH];
    GetSystemDirectoryA(path, MAX_PATH);   // SysWOW64 for a 32-bit process on 64-bit Windows
    lstrcatA(path, "\\dinput8.dll");
    g_real_dinput8 = LoadLibraryA(path);
    if (!g_real_dinput8) return E_FAIL;
  }
  auto fn = (DirectInput8Create_t)GetProcAddress(g_real_dinput8, "DirectInput8Create");
  if (!fn) return E_FAIL;
  return fn(inst, ver, riid, out, outer);
}

// ---------------------------------------------------------------- hook helper
struct Hook {
  void* trampoline = nullptr;
  void Create(void* target, void* detour) {
    MH_CreateHook(target, detour, &trampoline);
    MH_EnableHook(target);
  }
  void CreateInVMT(void* obj, size_t index, void* detour) {
    Create((*(void***)obj)[index], detour);
  }
};

static Hook g_create9, g_create_device, g_reset;
static HWND g_game_window = nullptr;

// ---------------------------------------------------------------- borderless logic
static void MakeBorderless(D3DPRESENT_PARAMETERS* pp, HWND fallback) {
  HWND window = nullptr;
  if (pp->hDeviceWindow && IsWindow(pp->hDeviceWindow)) window = pp->hDeviceWindow;
  else if (fallback && IsWindow(fallback))               window = fallback;
  else if (g_game_window && IsWindow(g_game_window))     window = g_game_window;
  else                                                   window = GetActiveWindow();
  if (!window) return;

  g_game_window = window;
  SetForegroundWindow(window);

  pp->Windowed = TRUE;
  pp->FullScreen_RefreshRateInHz = 0;
  pp->hDeviceWindow = window;

  LONG style = GetWindowLongA(window, GWL_STYLE);
  style &= ~(WS_OVERLAPPEDWINDOW | WS_MINIMIZE | WS_MAXIMIZE);
  style |= WS_POPUP;
  SetWindowLongA(window, GWL_STYLE, style);

  MONITORINFO mi = {};
  mi.cbSize = sizeof(mi);
  GetMonitorInfoA(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &mi);

  SetWindowPos(window, nullptr,
               mi.rcMonitor.left, mi.rcMonitor.top,
               mi.rcMonitor.right - mi.rcMonitor.left,
               mi.rcMonitor.bottom - mi.rcMonitor.top,
               SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOOWNERZORDER);
  BringWindowToTop(window);
}

// ---------------------------------------------------------------- D3D9 detours
typedef IDirect3D9* (WINAPI *Create9_t)(UINT);
typedef HRESULT (WINAPI *CreateDevice_t)(IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);
typedef HRESULT (WINAPI *Reset_t)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);

static HRESULT WINAPI Reset_Detour(IDirect3DDevice9* self, D3DPRESENT_PARAMETERS* pp) {
  if (pp && !pp->Windowed)
    MakeBorderless(pp, nullptr);
  return ((Reset_t)g_reset.trampoline)(self, pp);
}

static HRESULT WINAPI CreateDevice_Detour(IDirect3D9* self, UINT adapter, D3DDEVTYPE type, HWND focus,
                                          DWORD flags, D3DPRESENT_PARAMETERS* pp, IDirect3DDevice9** out) {
  if (focus) g_game_window = focus;
  if (pp && !pp->Windowed)
    MakeBorderless(pp, focus);

  HRESULT hr = ((CreateDevice_t)g_create_device.trampoline)(self, adapter, type, focus, flags, pp, out);

  if (SUCCEEDED(hr) && out && *out && !g_reset.trampoline)
    g_reset.CreateInVMT(*out, 16, (void*)&Reset_Detour);   // IDirect3DDevice9::Reset = vtable[16]
  return hr;
}

static IDirect3D9* WINAPI Create9_Detour(UINT sdk) {
  IDirect3D9* d3d = ((Create9_t)g_create9.trampoline)(sdk);
  if (d3d && !g_create_device.trampoline)
    g_create_device.CreateInVMT(d3d, 16, (void*)&CreateDevice_Detour);  // IDirect3D9::CreateDevice = vtable[16]
  return d3d;
}

// ---------------------------------------------------------------- startup
static DWORD WINAPI InitThread(LPVOID) {
  HMODULE d3d9 = nullptr;
  while (!(d3d9 = GetModuleHandleA("d3d9.dll")))
    Sleep(5);

  if (MH_Initialize() != MH_OK) return 0;
  FARPROC proc = GetProcAddress(d3d9, "Direct3DCreate9");
  if (proc) g_create9.Create((void*)proc, (void*)&Create9_Detour);
  return 0;
}

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(module);
    HANDLE t = CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
    if (t) CloseHandle(t);
  }
  return TRUE;
}
