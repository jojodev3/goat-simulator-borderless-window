# Goat Simulator Borderless Window

A small mod that adds borderless window mode to **Goat Simulator** (the original, 32-bit, Direct3D 9 version).

When the game asks for exclusive fullscreen, the mod turns it into a borderless window covering your monitor. Alt-tabbing then works normally.

## Install

1. Download `dinput8.dll` from the [releases](../../releases) page, or build it yourself (see below).
2. Open the Goat Simulator folder (Steam > right-click the game > Manage > Browse local files).
3. Copy `dinput8.dll` into `Binaries\Win32` (next to `GoatGame.exe`).
4. Start the game and set it to **Fullscreen** in the options. It will run as a borderless window.

To uninstall, delete `dinput8.dll` from that folder.

## How it works

The mod is a `dinput8.dll` proxy: the game loads it instead of the system DLL, and it forwards `DirectInput8Create` to the real one. Using [MinHook](https://github.com/TsudaKageyu/minhook) it hooks `Direct3DCreate9`, `IDirect3D9::CreateDevice` and `IDirect3DDevice9::Reset`. Whenever a fullscreen presentation is requested it switches the request to windowed, removes the window frame and resizes the window to the monitor.

The whole thing is one readable file: [`src/goat_borderless.cpp`](src/goat_borderless.cpp).

## Build

Requires MinGW-w64 targeting 32-bit (`i686-w64-mingw32-g++`):

```
make
```

The result is `build/dinput8.dll`. It must be 32-bit to match the game.

## Notes

- Single-player use only. The mod hooks graphics calls in the game process, so some antivirus programs may flag it heuristically. The source is here to read and build yourself.
- Only the original Goat Simulator has been considered. Other Goat Simulator titles may behave differently.

## Credits

- Based on the technique used by [lanylow/outlast-borderless-window](https://github.com/lanylow/outlast-borderless-window) (GPL-3.0).
- [MinHook](https://github.com/TsudaKageyu/minhook) by Tsuda Kageyu (BSD-2-Clause), included in `lib/minhook`.

## License

GPL-3.0, see [LICENSE](LICENSE). MinHook is under its own license, see `lib/minhook/LICENSE.txt`.
