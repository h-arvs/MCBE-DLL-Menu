# MCBE DLL Menu

Sigless ImGui rendering and input, via an injected DLL, for Minecraft: Bedrock Edition.

## Support

This project supports all GDK Minecraft: Bedrock Edition versions (1.21.122+).

## Feature Overview

- ImGui rendering (and input routing) with support for d3d11 and d3d12 (through d3d11on12)
- Sigless mouse input interception via GameInput APIs and hooks
- WndProc hook for keyboard input interception
- Console
- Safe UnInjection logic

## Build Requirements

- C++ compiler with C++ 23 support
- CMake 3.5 or above
- Windows 10 or 11 SDK for DirectX libraries and
  headers - [Download](https://learn.microsoft.com/en-us/windows/apps/windows-sdk/downloads)
- GameInput V2 header - [Download](https://github.com/microsoftconnect/GameInput/blob/main/include/v2/GameInput.h)
- Microsoft GDK for GameInput library - [Download](https://github.com/microsoft/GDK/releases/)

## Usage

- Build MCBEDllMenu.dll
- Inject dll into Minecraft.Windows.exe process via your favourite injector