# hSC Plugin
&emsp;**English** | [简体中文](https://github.com/HTMonkeyG/hSC/blob/main/README.md)

hSC Plugin is a dll plugin developed for Sky:CotL.
The plugin integrates coordinate modification, free camera and aerial camera operation,
provides assistance for in-game photography and video recording.

**The features within delete lines is under development, and may vary anytime.**

## 1 Features
Press `Insert` on the right of the keyboard to display in-game ui.
Press `Left-Alt` to display mouse cursor for operation.

The `Take over` check box at the top of the interface is the global switch.
If checked, the camera will be taken over by the plugin.

Select the camera mode to take over from the drop-down box below: `First person` is the first person handheld mode, `Front` is the front mode, and `Placed` is the tripod mode.

The selection column below is the operation type: `Set` is the direct setting mode, `Freecam` is the out of body mode, and `FPV` is the FPV simulation mode.

### 1.1 `Set`
After selecting this option, five input boxes will appear at the bottom,
including coordinates, orientation, focus, zoom and brightness settings.
You can enter numbers directly.

The input is valid only after the check box of the corresponding operation is checked.

This mode is applicable to static shooting and other situations.

### 1.2 `Freecam`
In this mode, you can use WASD, space and left shift to move the camera freely, and use the speed input box to adjust the moving speed.
~~The speed can also be adjusted in the range of 1-1000 using the mouse wheel.~~

When the `Axial` checkbox is checked, the horizontal movement of the camera will be parallel to the xOz plane.

### 1.3 `FPV`
**This option is under heavy development, all descriptions cannot guarantee stability and compatibility.**

In this mode, WASD and mouse can be used to simulate the operation of FPV for recording.

## 2 How to use
1. Download the latest precompiled dll in the releases.
2. Inject the dll into the game while the main login menu is displayed.
3. Press `Insert` to switch the in-game ui.

## 3 How to compile
The software is compiled with mingw64-15.1.0. Download Source Code (.zip), create a new folder named `dist` in the same directory of Makefile, and run `mingw32-make.exe` after decompression.

For the first compilation, run `mingw32-make.exe all` to compile the library file.

This software does not guarantee that it can be compiled and run on a lower version of MinGW or an operating system lower than win10 22h2. If you have any compatibility problems, please contact by email.

The MinHook used in this software has been modified to solve the hook failure that may be caused by the VMP protection of NetEase. Imgui has been modified to allow compilation under some MinGW SDK.

**Do not use third-party library versions that are not provided in the repository.**
